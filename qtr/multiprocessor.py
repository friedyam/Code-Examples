import click
from collections import defaultdict, OrderedDict
from configobj import ConfigObj
from ctypes import c_bool
import fnmatch
import glob
import inspect
import math
from multiprocessing import active_children, cpu_count, Manager, Pool, Process, Value
from operator import itemgetter
import os
import pytest
import sys
import time

# Some constants to use for results output, test result markers, and expected fields in the config file
CONFIG_FIELDS = {'home'}
TEST_FILE_PATH = 'src/test/config/testfilemapping.ini'
TMP_PATH = 'src/test/python/qtr/tmp'
TMP_RESULTS_PATH = 'src/test/python/qtr/tmp_results'
FAILED_TEST = 'F'
XFAILED_TEST = 'x'
PASSED_TEST = '.'
pool_dict = defaultdict(lambda: defaultdict())

# Context for the command-line program
class Context(object):
    def __init__(self):
        self.config_file = None
        self.config_path = None
        self.debug = None
        self.force_serial = None
        self.home = None
        self.results_path = None
        self.results_queue = None
        self.results_process = None
        self.more_results = None
        self.manager = None
        self.processes = {}
        self.tests = None
        self.test_file = None
        self.test_type = None

pass_context = click.make_pass_decorator(Context, ensure=True)

# Runs the specified test using pytest and sends the test result to the results queue
def test_worker(home, test, results_queue):
    pid = str(os.getpid())
    log_name = test.split('/')[-1].split('.')[0] + "_" + pid
    results_log = os.path.join(home, TMP_RESULTS_PATH, "results_{0}.txt".format(log_name))
    try:
    	# Since all test workers output to stdout at the same time, send that output to a temporary junk file
        old_stdout = sys.stdout
        sys.stdout = open(os.path.join(home, TMP_PATH, "tmp_{0}.txt".format(pid)), 'w')
        pytest.main("--result-log={0} {1}".format(results_log, os.path.join(home, test)))
        sys.stdout.close()
        sys.stdout = old_stdout
    # Clean up incase control-c is pressed
    except KeyboardInterrupt:
        if not sys.stdout.closed:
            sys.stdout.close()
        sys.stdout = old_stdout
        raise
    # Clean up if there is an IOError from the output file
    except IOError:
        if not sys.stdout.closed:
            sys.stdout.close()
        sys.stdout = old_stdout
        print "There's been an IOError"
        raise

    results_queue.put(log_name)

# Collect test results from the worker processes and display them to the console
def results_processor(results_queue, more_results, home, results_path):
    try:
        failed_results = []
        # Output results to a file if specified
        if results_path:
            results_logger = open(results_path, 'w')
        while more_results.value or not results_queue.empty():
            test_name = results_queue.get()
            glob_results = glob.glob(os.path.join(home, TMP_RESULTS_PATH, "results_{0}.txt".format(test_name)))
            if glob_results and os.path.isfile(glob_results[0]):
                result_abspath = glob_results[0]
                with open(result_abspath, 'r') as result_file:
                    test_result = result_file.readline().strip()
                    if test_result.startswith(FAILED_TEST):
                        click.secho(test_result, fg='red', bold=True)
                        failed_results.append(result_abspath)
                    elif test_result.startswith(XFAILED_TEST):
                        click.secho(test_result, fg='yellow')
                    elif test_result.startswith(PASSED_TEST):
                        click.secho(test_result, fg='green')
                    if results_path:
                        result_file.seek(0)
                        results_logger.write(result_file.read())
                if not test_result.startswith(FAILED_TEST):
                    os.remove(result_abspath)
            else:
                click.secho(test_name.strip(), fg='white', bold=True)
        # Output the tracebacks of the failed tests to the console
        for failed_abspath in failed_results:
            with open(failed_abspath, 'r') as failed:
                click.secho(failed.readline().strip(), fg='white', bold=True)
                click.secho(failed.read(), fg='red')
            os.remove(failed_abspath)
        if results_path:
            results_logger.close()
        results_queue.put(len(failed_results))
    except IOError:
        print "Unexpected file error"
        if not results_logger.closed:
            results_logger.close()
        while not results_queue.empty():
            results_queue.get()
        results_queue.put(-1)
        raise


def load_config(ctx):
    if not os.path.isfile(ctx.config_path):
        raise click.UsageError("Cannot find config file. Run 'qtr config' to create a new config file")
    ctx.config_file = ConfigObj(ctx.config_path)
    if set(ctx.config_file.keys()) != CONFIG_FIELDS:
        raise click.UsageError("Incorrect config file format. Run 'qtr config' to create a new config file")
    ctx.home = ctx.config_file['home']



def load_tests(ctx):
    num_tests = 0
    test_dict = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: list())))
    wildcard_tests = []
    for i in reversed(xrange(len(ctx.tests))):
        if '*' in ctx.tests[i] or '?' in ctx.tests[i]:
            wildcard_tests.append(ctx.tests[i])
            del ctx.tests[i]
    for sec in ctx.test_file:
        for subsec in ctx.test_file[sec]:
            subsec_tests, subsec_details = zip(*ctx.test_file[sec][subsec].items())
            for pattern in wildcard_tests:
                ctx.tests += fnmatch.filter(subsec_tests, pattern)
            for test, details in zip(subsec_tests, subsec_details):
                if (not ctx.tests or test in ctx.tests) and (not ctx.test_type or details[1] == ctx.test_type):
                    if not ctx.force_serial:
                        test_dict[sec][subsec][details[1]].append((ctx.home, details[0], ctx.results_queue))
                    else:
                        test_dict['Serial']['Serial'][details[1]].append((ctx.home, details[0], ctx.results_queue))
                    num_tests += 1
    return test_dict, num_tests

# 
def run_tests(ctx, test_dict):
    for test_group in test_dict['OneFromGroup']:
        for test_type in test_dict['OneFromGroup'][test_group]:
            pool_dict[test_group][test_type] = Pool(processes=1)
            ctx.processes[test_type] -= 1
            for args in test_dict['OneFromGroup'][test_group][test_type]:
                pool_dict[test_group][test_type].apply_async(func=test_worker, args=args)
    for test_group in test_dict['Parallel']:
        for test_type in test_dict['Parallel'][test_group]:
            pool_dict[test_group][test_type] = Pool(processes=ctx.processes[test_type])
            ctx.processes[test_type] = 0
            for args in test_dict['Parallel'][test_group][test_type]:
                pool_dict[test_group][test_type].apply_async(func=test_worker, args=args)

    if 'Parallel' in pool_dict.keys():
        for pool_type in pool_dict['Parallel']:
            pool_dict['Parallel'][pool_type].close()
            pool_dict['Parallel'][pool_type].join()
        del pool_dict['Parallel']

    # incorrect, selenium is not necessarily at 15
    ctx.processes['API'] = cpu_count()
    ctx.processes['SELENIUM'] = 15

    for test_group in test_dict['Serial']:
        for test_type in test_dict['Serial'][test_group]:
            pool_dict[test_group][test_type] = Pool(processes=1)
            ctx.processes[test_type] -= 1
            for args in test_dict['Serial'][test_group][test_type]:
                pool_dict[test_group][test_type].apply_async(func=test_worker, args=args)
    for pool_type in pool_dict.keys():
        for pool in pool_dict[pool_type].values():
            pool.close()
            pool.join()

# Download test times from sauce labs 
def get_test_times(num_tests):
    from sauceutils import SauceTools
    sauce = SauceTools("https://saucelabs.com", "polarqa", "d609b648-22e3-44bb-a38e-c28931df837d")
    jobs = []
    last_time = int(time.time())
    test_times = defaultdict(list)
    bar_length = int(math.ceil(num_tests/500))

    with click.progressbar(xrange(bar_length),
                           label="Downloading statistics from Sauce Labs",
                           fill_char=click.style('+', fg='green', bold=True),
                           empty_char=click.style('-', fg='red'),
                           width=40) as bar:
        for i in bar:
            jobs += sauce.get_jobs(num_jobs=500, full=True, end_date=last_time)
            last_time = int(jobs[-1]['start_time'])

    # Only add tests that have passed
    for job in jobs:
        if job['passed'] and job['end_time']:
            test_times[job['name'].lower()].append([float(job['creation_time']),
                                                    float(job['end_time']) - float(job['start_time'])])
    click.secho("Sorted through statistics", fg='white')

    return test_times

def weighter(times_list):
	# Sort tests by newest to oldest test run date
    times_list.sort(key=itemgetter(0), reverse=True)
    n = len(times_list)
    # Apply a weight from 1/n up to 1 for each test
    for i in xrange(n):
        weight = math.pow(float(n-i)/n, 2)
        times_list[i][1] *= weight
        times_list[i].append(weight)


def average_and_sort(times_dict):
    weighted_avg_list = []
    for key in times_dict.keys():
    	# Apply weights to a list of times for a single test
        weighter(times_dict[key])
        # Add the test name and the weighted average time to list of test times
        weighted_avg_list.append([key, math.fsum([i[1] for i in times_dict[key]]) / math.fsum([i[2] for i in times_dict[key]])])
    click.secho("Applied weighted average to test times", fg='white')

    # Sort tests by longest average test time
    weighted_avg_list.sort(key=itemgetter(1), reverse=True)
    click.secho("Sorted through test times", fg='white')
    return weighted_avg_list


@click.group()
@click.option('--config-path',
              default=os.path.join(os.path.expanduser('~'), ".qtr"),
              help="Specify a config file path other than the default (your home directory)")
@click.option('--debug',
              is_flag=True,
              help='Enable debugging')
@pass_context
def cli(ctx, config_path, debug):
    """QA Test Runner or qtr is used to run all QA tests in parallel or serial"""
    ctx.config_path = config_path
    ctx.debug = debug


@cli.command()
@click.option('-h',
              '--home',
              prompt="Specify the location of the polar.selenium repository",
              type=click.Path(exists=True,
                              file_okay=False,
                              resolve_path=True),
              help="The location of your local copy of the polar.selenium repository")
@pass_context
def config(ctx, home):
    """Creates a config file for you in your home directory"""
    frame = inspect.currentframe()
    args, _, _, values = inspect.getargvalues(frame)
    args.remove('ctx')
    config_raw = ['{0}={1}'.format(arg, values[arg]) for arg in args]
    with open(ctx.config_path, 'w') as config_file:
        config_file.write("\n".join(config_raw))


@cli.command()
@click.option('-s',
              '--selenium-processes',
              default=15,
              type=click.IntRange(1),
              help='The number of processes to run the Selenium tests with')
@click.option('-a',
              '--api-processes',
              default=cpu_count(),
              type=int,
              help='The number of processes to run the API tests with')
@click.option('--force-serial',
              default=False,
              is_flag=True,
              help='Forces all tests to run in serial (one at a time)')
@click.option('--test-type',
              default=None,
              type=click.Choice(['SELENIUM', 'API']),
              help='Specify which type of tests you want to run. For all types, exclude flag')
@click.option('-r',
              '--results-path',
              default=None,
              type=str,
              help='The location to store test results')
@click.argument('tests',
                nargs=-1,
                default=None)
@pass_context
def run(ctx, selenium_processes, api_processes, force_serial, test_type, results_path, tests):
    """Runs all QA tests in parallel or serial.

    \b
    `qtr run` will run all tests in the test config file.
    [TESTS] arguments should be the file name of the test
    you want to run, without the .py extension.

    [TESTS] arguments work with Unix shell-style wildcards i.e. 'testc14*'.
    Each wildcard style argument must be enclosed in single quotes"""
    # Add command-line parameters to context
    ctx.processes['SELENIUM'] = selenium_processes
    ctx.processes['API'] = api_processes
    ctx.force_serial = force_serial
    ctx.test_type = test_type
    ctx.results_path = results_path
    load_config(ctx)
    ctx.test_file = ConfigObj(os.path.join(ctx.home, TEST_FILE_PATH))
    ctx.tests = list(tests)
    # Create temporary folders to collect test results and other data
    if not os.path.exists(os.path.join(ctx.home, TMP_PATH)):
        os.mkdir(os.path.join(ctx.home, TMP_PATH))
    if not os.path.exists(os.path.join(ctx.home, TMP_RESULTS_PATH)):
        os.mkdir(os.path.join(ctx.home, TMP_RESULTS_PATH))
    # Create results queue and results queue flag
    ctx.manager = Manager()
    ctx.more_results = Value(c_bool, True)
    ctx.results_queue = ctx.manager.Queue()
    ctx.results_process = Process(target=results_processor,
                                  args=(ctx.results_queue,
                                        ctx.more_results,
                                        ctx.home,
                                        ctx.results_path))
    ctx.results_process.start()
    # Load the appropriate tests and output number of tests
    test_dict, num_tests = load_tests(ctx)
    click.secho("Collected {0} test(s)".format(num_tests), fg='white', bold=True)
    # Run tests and collect timing data
    start_time = time.time()
    run_tests(ctx, test_dict)
    end_time = time.time()
    # Kill the results queue by setting the flag to false
    with ctx.more_results.get_lock():
        ctx.results_queue.put("Testing is finished")
        ctx.more_results.value = False
    ctx.results_process.join()
    # Get the number of failed tests and output test results and timing data
    num_failed_tests = ctx.results_queue.get()
    if num_failed_tests == 0:
        click.secho("All tests passed!", fg='green', bold=True)
    elif num_failed_tests > 0:
        click.secho("Number of failed tests: {}".format(num_failed_tests), fg='red', bold=True)
    else:
        click.secho("Results process failed.", fg='red', bold=True)
    click.secho("Tests took {0} seconds".format(str(end_time - start_time)), fg='white', bold=True)
    # Clean up temporary files
    tmp_file_paths = glob.glob(os.path.join(ctx.home, TMP_PATH, "*"))
    for file_path in tmp_file_paths:
        os.remove(file_path)
    sys.exit(0) if num_failed_tests == 0 else sys.exit(1)

@cli.command()
@pass_context
def reschedule(ctx):
    """Rearranges the Selenium tests in the config file so that they will be run in a more efficient way"""
    load_config(ctx)
    ctx.test_file = ConfigObj(os.path.join(ctx.home, TEST_FILE_PATH))
    sys.path.insert(1, os.path.join(ctx.home, 'src/test/python/utility'))
    new_parallel_test_order = OrderedDict()
    # Get timing data from sauce labs
    test_times_dict = get_test_times(10000)

    # Apply a weight to test times and sort by shortest test times
    test_weighted_avg_list = average_and_sort(test_times_dict)

    # Rewrite the parallel test section of the config file
    parallel_section = ctx.test_file['Parallel']['Parallel']
    parallel_tests = parallel_section.keys()
    new_parallel_order = [i[0] for i in test_weighted_avg_list if i[0] in parallel_tests]
    for test in new_parallel_order:
        new_parallel_test_order[test] = parallel_section[test]
    if len(parallel_tests) > len(new_parallel_order):
        missing_tests = set(parallel_tests) - set(new_parallel_order)
        for test in missing_tests:
            new_parallel_test_order[test] = parallel_section[test]
    ctx.test_file['Parallel']['Parallel'] = new_parallel_test_order
    ctx.test_file.write()
    click.secho("Rewrote config file", fg='white')

if __name__ == '__main__':
    cli()


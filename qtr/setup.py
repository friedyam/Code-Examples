from setuptools import setup

setup(
    name='qtr',
    version='0.1.3',
    py_modules=['multiprocessor'],
    install_requires=[
        'Click',
        'Pytest',
        'multiprocessing'
    ],
    entry_points='''
        [console_scripts]
        qtr=multiprocessor:cli
    ''',
)

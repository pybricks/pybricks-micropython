"""pypi compatible setup module.

This setup is based on:
https://packaging.python.org/tutorials/distributing-packages/


"""
from setuptools import setup
from codecs import open
from os import path


here = path.abspath(path.dirname(__file__))
with open(path.join(here, 'README.rst'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='pybricks',
    version='0.1',
    description='Pybricks',
    long_description=long_description,
    url='https://github.com/pybricks',
    author='Laurens Valk and David Lechner',
    author_email='laurensvalk@gmail.com',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
    ],
    keywords='lego mindstorms ev3',
    python_requires='>=3',
    packages=['pybricks'],
)

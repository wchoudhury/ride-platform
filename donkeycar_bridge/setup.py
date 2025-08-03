from setuptools import setup, find_packages

setup(
    name="donkeycar_bridge",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "donkeycar>=4.3.0",
        "numpy>=1.18.5",
        "pybind11>=2.6.0",
    ],
    author="RIDE Project",
    author_email="ride@example.com",
    description="Bridge between Donkeycar API and CPM Lab",
    keywords="donkeycar, cpm, autonomous vehicles",
    python_requires=">=3.7",
)
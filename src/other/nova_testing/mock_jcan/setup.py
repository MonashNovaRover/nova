from setuptools import setup, find_packages

setup(
    name="jcan",
    version="0.0.0",
    description="Pure-python mock of the JCAN library (jcan_python) for use in tests, "
                "without requiring the compiled Rust extension or real/virtual CAN hardware.",
    packages=find_packages(include=["jcan", "jcan.*"]),
    python_requires=">=3.8",
    zip_safe=True,
)

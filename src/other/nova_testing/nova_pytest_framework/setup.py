from setuptools import setup, find_packages

setup(
    name="nova-pytest-framework",
    version="0.0.0",
    description="Shared pytest fixtures for Nova ROS 2 integration tests. "
                "Provides TesterNode, SUT executor, and mock JCAN reset helpers.",
    packages=find_packages(include=["nova_pytest_framework", "nova_pytest_framework.*"]),
    package_data={"nova_pytest_framework": ["py.typed"]},
    python_requires=">=3.8",
    install_requires=[
        "pytest",
    ],
    entry_points={
        "pytest11": [
            "nova_pytest_framework = nova_pytest_framework.plugin",
        ],
    },
    zip_safe=True,
)

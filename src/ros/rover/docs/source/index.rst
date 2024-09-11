.. rover documentation master file, created by
   sphinx-quickstart on Sun Feb 12 17:25:24 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Nova Rover's Documentation for 2023!
================================================

This documentation was automatically built using a basic Sphinx documentation project in the ``rover/docs`` folder. 
The content in this landing page was formatted using re-structure text (rst), which can be found in ``~/nova_ws/src/rover/docs/source/index.rst``.
All the other content was auto generated using ``sphinx-apidoc``. To build documentation, you will first need to run the following::

        sudo apt-get install python3-sphinx
        pip install sphinx-autodocgen
        pip install sphinx_rtd_theme

Then, make sure you've sourced the ``rover/core/aliases.sh`` so that you can run the ``docgen`` command.


Giving modules the gift of documentation
------------------------------------------

Currently, only autonomous has automatically generated documentation. 

For sphinx-autodoc to generate documentation for a new module, we need to do the following:

- Install the required module. Currently, only autonomous has a ``setup.py``. Once other modules have a ``setup.py``, we can add::

          pip install -e ~/nova_ws/folder/with/setup.py

to the ``docgen`` macro. This is important because sphinx needs to import modules in order to generate documentation.

- Format function docstrings like this::

        """[Summary]
        :param [ParamName]: [ParamDescription], defaults to [DefaultParamVal]
        :type [ParamName]: [ParamType](, optional)
        ...
        :raises [ErrorType]: [ErrorDescription]
        ...
        :return: [ReturnDescription]
        :rtype: [ReturnType]
        """
This will allow Sphinx to make.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   autonomous
   control
   science
   core
   electronics


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

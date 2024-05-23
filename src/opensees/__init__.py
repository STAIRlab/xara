__version__  = "0.0.46"

# from .patch import layer

# Imports for this module
import math
import fnmatch
# from . import pattern
from .library.obj import *
from .library import pattern

from .library.model import model

import importlib

# something to compare the output of model.analyze to:
successful = 0


_libs = ["model", "uniaxial", "element", "backbone", "pattern", "constraint"]

def __getattr__(name):
    # For reference:
    #   https://peps.python.org/pep-0562/#id4
    if name in _libs:
        try:
            return getattr(importlib.import_module(".library", __name__), name)

        except AttributeError:
            raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
    else:
        try:
            return importlib.import_module("." + name, __name__)
        except:
            raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")



def instance(model):
    try:
        return model.__enter__()

    except AttributeError:
        return model.getCopy()




import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst


def gst_structure_to_dict(structure: Gst.Structure) -> dict[str, object]:
    output: dict[str, object] = {}
    for n in range(structure.n_fields()):
        field_name = structure.nth_field_name(n)
        value = structure.get_value(field_name)
        if isinstance(value, Gst.Structure):
            output[field_name] = gst_structure_to_dict(value)
        else:
            output[field_name] = structure.get_value(field_name)
    return output

import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst, GObject


def gst_structure_to_dict(structure: Gst.Structure) -> dict[str, object]:
    output: dict[str, object] = {}
    for n in range(structure.n_fields()):
        field_name = structure.nth_field_name(n)
        field_type = structure.get_field_type(field_name)
        if field_type == GObject.TYPE_INVALID:
            # Some fields have invalid types, and cannot be used.
            # For example: https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/issues/350
            output[field_name] = None
            continue
        output[field_name] = gst_value_to_py(structure.get_value(field_name))
    return output


def gst_array_to_list(array: Gst.ValueArray) -> list[object]:
    return [gst_value_to_py(Gst.ValueArray.get_value(array, i)) for i in range(Gst.ValueArray.get_size(array))]


def gst_value_to_py(value: object) -> object:
    if isinstance(value, Gst.Structure):
        return gst_structure_to_dict(value)
    if isinstance(value, Gst.ValueArray):
        return gst_array_to_list(value)
    if isinstance(value, GObject.GEnum):
        return value.value_nick

    return value

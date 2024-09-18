import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst, GObject


def dict_to_gst_structure(name: str, dictionary: dict[str, object]) -> Gst.Structure:
    structure = Gst.Structure.new_empty(name)
    for key, value in dictionary.items():
        gst_value: object
        if isinstance(value, dict):
            structure.set_value(key, dict_to_gst_structure(key, value))
        elif isinstance(value, list):
            array = GObject.ValueArray.new(len(value))
            for item in value:
                array.append(item)
            structure.set_array(key, array)
        else:
            structure.set_value(key, value)
    return structure


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
    return list(map(gst_value_to_py, array))


def gst_value_to_py(value: object) -> object:
    if isinstance(value, Gst.Structure):
        return gst_structure_to_dict(value)
    if isinstance(value, Gst.ValueArray):
        return gst_array_to_list(value)
    if isinstance(value, GObject.GEnum):
        return value.value_nick

    return value

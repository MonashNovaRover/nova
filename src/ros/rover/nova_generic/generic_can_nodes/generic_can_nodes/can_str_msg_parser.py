import jcan

def parse(msg: str) -> jcan.Frame:
    """
    Auxiliary function that parses a string CAN frame and returns a jcan.Frame
    """
    frame_id, data_str = msg.split("#")

    assert len(frame_id) == 3 and len(data_str) % 2 == 0, f"{msg} has an invalid format"

    data = [int(data_str[i:i+2], 16) for i in range(0, len(frame_id), 2)]

    return jcan.Frame(int(frame_id, 16), data)


if __name__=="__main__":
    print(parse("00F#FFFF"))

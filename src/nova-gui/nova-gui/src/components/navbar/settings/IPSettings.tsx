import {
  Button,
  Input,
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { useEffect, useState } from "react";
import toast from "react-hot-toast";
import { isIPAddress } from "../../../utils/regexUtils.ts";

/**
 * IP Settings component containing UI for setting IP addresses.
 * @constructor
 */
export function IPSettings() {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const [baseStationIP, setBaseStationIP] = useState<string>(
    uiState.baseStationIP
  );

  const [roverIP, setRoverIP] = useState<string>(uiState.roverIP);

  useEffect(() => {
    if (uiState.baseStationIP != baseStationIP)
      setBaseStationIP(uiState.baseStationIP);

    if (uiState.roverIP != roverIP)
      setRoverIP(uiState.roverIP);

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [uiState.baseStationIP, uiState.roverIP]);

  const submit = () => {
    if (!isIPAddress(baseStationIP) || !isIPAddress(roverIP)) {
      toast.error("Invalid IP address");
      return;
    }

    uiActions.updateIP(baseStationIP, roverIP);

    closeModal();
  };

  const onBaseIPChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setBaseStationIP(event.target.value);
  };

  const onRoverIPChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setRoverIP(event.target.value);
  };

  const closeModal = () => uiActions.setSettingsModal(false);

  return (
    <div className="flex flex-col gap-3">
      <Input
        fullWidth
        label="Base IP"
        value={baseStationIP}
        type="text"
        onChange={onBaseIPChange}
        isInvalid={!isIPAddress(baseStationIP)}
      />
      <Input
        fullWidth
        label="Rover IP"
        value={roverIP}
        type="text"
        onChange={onRoverIPChange}
        isInvalid={!isIPAddress(roverIP)}
      />

      <div className="flex flex-row">
        <div className="grow"></div>
        <Button size="sm" color="success" variant="flat" onPress={submit}>
          Submit
        </Button>
      </div>

    </div>
  );
}

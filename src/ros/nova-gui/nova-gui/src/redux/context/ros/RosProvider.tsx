import { useState, useEffect } from "react";
import { useSelector } from "react-redux";
import { Ros } from "roslib";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { RootState } from "../../RootState";
import { useBifrost } from "../../actions/bifrost/useBifrostAction";
import { BifrostConnectionStatus } from "../../models/bifrost/BifrostTypes";
import { RosContext } from "./RosContext";
import * as ROSLIB from "roslib";
import toast from "react-hot-toast";

export const RosProvider = (props: { children: React.ReactNode }) => {
  const uiStore = useSelector((state: RootState) => state.uiState);

  const bifrostStateStore = useSelector((state: RootState) => state.bifrostStatus);
  const [ros, setRos] = useState<ROSLIB.Ros | undefined>();
  const [wasConnected, setWasConnected] = useState(false);
  const bifrostActions = useBifrost({ topic: RosTopic.NULL_TOPIC });

  useEffect(() => {
    if (bifrostStateStore.connectionStatus !== BifrostConnectionStatus.DISCONNECTED) return;

    // Clean up old connection
    if (ros) {
      ros.close();
    }

    // Delay reconnection if we were previously connected
    const reconnectDelay = wasConnected ? 2000 : 0;

    const timeoutId = setTimeout(() => {
      const newRos = new Ros({ url: "ws://" + uiStore.baseStationIP + ":9090" });
      bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTING);

      newRos.on("connection", () => {
        bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTED);
        if (wasConnected) {
          toast.success("Reconnected to rosbridge");
        }
        setWasConnected(true);
      });

      newRos.on("error", () => {
        bifrostActions.updateBifrostConnection(BifrostConnectionStatus.DISCONNECTED);
      });

      newRos.on("close", () => {
        bifrostActions.updateBifrostConnection(BifrostConnectionStatus.DISCONNECTED);
      });

      setRos(newRos);
    }, reconnectDelay);

    return () => clearTimeout(timeoutId);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [uiStore.baseStationIP, bifrostStateStore.connectionStatus]);

  return <RosContext.Provider value={ros}>{props.children}</RosContext.Provider>;
};

import { useState, useEffect } from "react";
import { useSelector } from "react-redux";
import { Ros } from "roslib";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { RootState } from "../../RootState";
import { useBifrost } from "../../actions/bifrost/useBifrostAction";
import { BifrostConnectionStatus } from "../../models/bifrost/BifrostTypes";
import { RosContext } from "./RosContext";
import * as ROSLIB from "roslib";

export const RosProvider = (props: { children: React.ReactNode }) => {
  const uiStore = useSelector((state: RootState) => state.uiState);

  const bifrostStateStore = useSelector((state: RootState) => state.bifrostStatus);
  const [ros, setRos] = useState<ROSLIB.Ros | undefined>();
  const bifrostActions = useBifrost({ topic: RosTopic.NULL_TOPIC });

  useEffect(() => {
    if (bifrostStateStore.connectionStatus !== BifrostConnectionStatus.DISCONNECTED) return;
    const ros = new Ros({ url: "ws://" + uiStore.baseStationIP + ":9090" });
    bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTING);

    ros.on("connection", () => {
      bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTED);
    });

    ros.on("error", () => {
      bifrostActions.updateBifrostConnection(BifrostConnectionStatus.DISCONNECTED);
    });

    ros.on("close", () => {
      bifrostActions.updateBifrostConnection(BifrostConnectionStatus.DISCONNECTED);
    });

    setRos(ros);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [uiStore.baseStationIP]);

  return <RosContext.Provider value={ros}>{props.children}</RosContext.Provider>;
};

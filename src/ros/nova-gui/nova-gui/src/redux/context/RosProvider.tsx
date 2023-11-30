import { useState, useEffect } from "react";
import { useSelector } from "react-redux";
import { Ros } from "roslib";
import { RosTopics } from "../../ros/rosTopics";
import { RootState } from "../RootState";
import { useBifrost } from "../actions/useBifrostAction";
import { BifrostConnectionStatus } from "../models/BifrostTypes";
import { RosContext } from "./RosContext";

export const RosProvider = (props: { children: React.ReactNode }) => {
  const uiStore = useSelector((state: RootState) => state.uiState);
  const [ros, setRos] = useState<ROSLIB.Ros | undefined>();
  const bifrostActions = useBifrost(RosTopics.DEMO_TOPIC);

  useEffect(() => {
    bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTING);
    const ros = new Ros({ url: uiStore.rosUrl });

    ros.on("connection", () => {
      bifrostActions.updateBifrostConnection(BifrostConnectionStatus.CONNECTED);
    });

    ros.on("error", () => {
      bifrostActions.updateBifrostConnection(
        BifrostConnectionStatus.DISCONNECTED
      );
    });

    ros.on("close", () => {
      bifrostActions.updateBifrostConnection(
        BifrostConnectionStatus.DISCONNECTED
      );
    });

    setRos(ros);
  }, [uiStore.rosUrl]);

  return (
    <RosContext.Provider value={ros}>{props.children}</RosContext.Provider>
  );
};

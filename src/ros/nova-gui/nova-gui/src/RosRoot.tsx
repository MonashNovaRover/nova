import React, { createContext, useEffect, useState } from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { NovaNavbar } from "./components/navbar/Navbar";
import { useBifrost } from "./redux/actions/useBifrostAction";
import ROSLIB, { Ros } from "roslib";
import { BifrostConnectionStatus } from "./redux/models/BifrostTypes";
import { useSelector } from "react-redux";
import { RootState } from "./redux/RootState";
import { RosTopics } from "./ros/rosTopics";

export const RosContext = createContext<ROSLIB.Ros | undefined>(undefined);

const RosProvider = (props: { children: React.ReactNode }) => {
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

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground  w-screen h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
        <NovaNavbar />
        <SettingsModal />
        <Outlet />
      </div>
    </RosProvider>
  );
};

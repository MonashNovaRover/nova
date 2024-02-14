import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { NovaNavbar } from "./components/navbar/Navbar";
import { RosProvider } from "./redux/context/RosProvider";
import ControllerHelpModal from "./components/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground  bg-black">
        <NovaNavbar />
        <SettingsModal />
        <ControllerHelpModal/>
        <Outlet />
        <Toaster />
      </div>
    </RosProvider>
  );
};

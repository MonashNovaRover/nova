import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { NovaNavbar } from "./components/navbar/Navbar";
import Sidebar from "./components/navbar/Sidebar";
import { RosProvider } from "./redux/context/RosProvider";
import ControllerHelpModal from "./components/ControllerHelpModal/ControllerHelpModal";

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground w-screen h-screen [background:#131313]">
        <NovaNavbar />
        <div className="flex h-full">
          <Sidebar />
          <Outlet />
        </div>
        <SettingsModal />
        <ControllerHelpModal />
      </div>
    </RosProvider>
  );
};

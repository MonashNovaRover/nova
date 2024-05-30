import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { RosProvider } from "./redux/context/ros/RosProvider";
import ControllerHelpModal from "./components/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";
import { NovaTopBar } from "./components/TopBar/TopBar";
import { NeoSidebar } from "./components/NeoSidebar/NeoSidebar";
import { BLCMDStatusModal } from "./components/BLCMDStatusModal/BLCMDStatusModal";

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground h-full min-h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
        <NovaTopBar />
        <NeoSidebar />
        <Outlet />
        {/* All (ahh Most) Modals Here */}
        <SettingsModal />
        <ControllerHelpModal />
        <BLCMDStatusModal />
        <Toaster />
      </div>
    </RosProvider>
  );
};

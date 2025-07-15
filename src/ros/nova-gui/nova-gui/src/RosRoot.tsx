import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/navbar/settings/SettingsModal";
import { RosProvider } from "./redux/context/ros/RosProvider";
import ControllerHelpModal from "./components/navbar/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";
import { NovaTopBar } from "./components/navbar/TopBar/TopBar";
import { NeoSidebar } from "./components/navbar/NeoSidebar/NeoSidebar";
import { BLCMDStatusModal } from "./components/navbar/BLCMDStatusModal/BLCMDStatusModal";

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

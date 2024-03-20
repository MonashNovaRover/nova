import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { RosProvider } from "./redux/context/RosProvider";
import ControllerHelpModal from "./components/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";
import { NovaNavbar } from "./components/navbar/Navbar";
import { NeoSidebar } from "./components/NeoSidebar/NeoSidebar";
import { BLCMDStatusModal } from "./components/BLCMDStatusModal/BLCMDStatusModal";

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground w-screen h-full min-h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
        <NovaNavbar />
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

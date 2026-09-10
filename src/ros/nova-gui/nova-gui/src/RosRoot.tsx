import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/navbar/settings/SettingsModal";
import { RosProvider } from "./redux/context/ros/RosProvider";
import ControllerHelpModal from "./components/navbar/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";
import { NovaTopBar } from "./components/navbar/TopBar/TopBar";
import { NeoSidebar } from "./components/navbar/NeoSidebar/NeoSidebar";
import { BLCMDStatusModal } from "./components/navbar/BLCMDStatusModal/BLCMDStatusModal";
import { useSelector } from "react-redux";
import type { RootState } from "./redux/RootState";

export const RosRoot: React.FC = () => {
  const uiState = useSelector((state: RootState) => state.uiState);

  return (
    <RosProvider>
      <div className="dark text-foreground h-full min-h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_50%,#F770AD_100%)]">
        <NovaTopBar />
        <NeoSidebar />
        <Outlet />
        {/* All (ahh Most) Modals Here */}
        {uiState.settingsModalOpen && <SettingsModal />}
        {uiState.controllerHelpModalOpen && <ControllerHelpModal />}
        {uiState.blcmdStatusModalOpen && <BLCMDStatusModal />}
        <Toaster />
      </div>
    </RosProvider>
  );
};

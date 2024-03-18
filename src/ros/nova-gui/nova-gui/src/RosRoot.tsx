import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { RosProvider } from "./redux/context/RosProvider";
import ControllerHelpModal from "./components/ControllerHelpModal/ControllerHelpModal";
import { Toaster } from "react-hot-toast";
import { NovaNavbar } from "./components/navbar/Navbar";
import { NeoSidebar } from "./components/NeoSidebar/NeoSidebar";

export const RosRoot: React.FC = () => {
  return (
    <RosProvider>
      <div className="dark text-foreground w-screen h-full min-h-screen [background:#131313]">
        <NovaNavbar />
        <NeoSidebar />

        {/* <Sidebar /> */}
        <Outlet />
        <SettingsModal />
        <ControllerHelpModal />
        <Toaster />
      </div>
    </RosProvider>
  );
};

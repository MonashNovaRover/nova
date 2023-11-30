import React from "react";
import { Outlet } from "react-router-dom";
import { SettingsModal } from "./components/settings/SettingsModal";
import { NovaNavbar } from "./components/navbar/Navbar";
import { RosProvider } from "./redux/context/RosProvider";

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

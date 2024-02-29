import React, { ReactElement } from "react";
import { Link } from "react-router-dom";

interface SidebarButtonProps {
  icon: ReactElement;
  text: string;
  to: string;
  isActive: boolean;
}

const SidebarButton: React.FC<SidebarButtonProps> = ({ icon, to, isActive }) => {
  return (
    <Link to={to} className="text-white text-3xl flex justify-center items-center">
      <div className={`flex flex-col items-center mt-4 ${isActive ? " flex justify-center items-center bg-292929 shadow-md rounded-md w-12 h-12" : ""}`}>
        <div className={`p-2 w-9 h-9 flex justify-center items-center text-10xl`}>
          {icon}
        </div>
      </div>
    </Link>
  );
};

export default SidebarButton;

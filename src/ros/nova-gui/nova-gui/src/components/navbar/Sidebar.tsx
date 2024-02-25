import React from "react";
import { useLocation } from "react-router-dom";
import SidebarButton from "./SidebarButton";
import { Camera, House, Map, GraphUp, RocketTakeoff, BoxSeam, ConeStriped, Envelope, WrenchAdjustable } from "react-bootstrap-icons";

const iconMap: Record<string, Record<string, JSX.Element>> = {
  arc: {
    "/arc": <House />,
    "/arc/post-landing": <RocketTakeoff />,
    "/arc/space-resources": <BoxSeam />,
    "/arc/excavation-construction": <ConeStriped />,
    "/arc/mapping-autonomous": <Map />,
  },
  urc: {
    "/urc": <House />,
    "/urc/science": <GraphUp />,
    "/urc/delivery": <Envelope />,
    "/urc/equipment-servicing": <WrenchAdjustable />,
    "/urc/autonomous-navigation": <Map />,
  },
};

const Sidebar: React.FC = () => {
  const location = useLocation();
  const currentPath = location.pathname;
  const routePrefix = currentPath.split("/")[1]; 

  // Get the icon map based on the current route prefix
  const routeIcons = iconMap[routePrefix];

  const displayAllButtons = !routeIcons;

  return (
    <div className="bg-[#1A1A1A] h-full w-16 p-4">
      {displayAllButtons &&
        Object.values(iconMap).flatMap(routeIcons =>
          Object.entries(routeIcons).map(([path, icon], index) => (
            <SidebarButton
              key={index}
              icon={icon}
              text={path}
              to={path}
              isActive={currentPath === path}
            />
          ))
        )}
      {!displayAllButtons && 
        Object.entries(routeIcons).map(([path, icon], index) => (
          <SidebarButton
            key={index}
            icon={icon}
            text={path}
            to={path}
            isActive={currentPath === path}
          />
        ))}
    </div>
  );
};

export default Sidebar;

import React, { useState } from "react";
import { useLocation } from "react-router-dom";
import { Map, RocketTakeoff, BoxSeam, ConeStriped } from "react-bootstrap-icons";
import "./Sidebar.css";
import { RootState } from "../../redux/RootState";
// import { useUIActions } from "../../redux/actions/useUIActions";
import { useSelector } from "react-redux";

const sidebarData = {
  arc: [
    {
      title: "Post Landing",
      link: "arc/post_landing",
      icon: <RocketTakeoff />,
      children: [
        {
          title: "Cameras",
          link: "/cameras",
        },
        {
          title: "Status Monitor",
          link: "/status-monitor",
        },
      ],
    },
    {
      title: "Space Resources",
      link: "arc/space_resources",
      icon: <BoxSeam />,
      children: [
        {
          title: "Cameras",
          link: "/cameras",
        },
        {
          title: "Status Monitor",
          link: "/status-monitor",
        },
        {
          title: "Microscope",
          link: "/microscope",
        },
      ],
    },
    {
      title: "Construction Excavation",
      link: "arc/construction_excavation",
      icon: <ConeStriped />,
      children: [
        {
          title: "Cameras",
          link: "/cameras",
        },
        {
          title: "Status Monitor",
          link: "/status-monitor",
        },
      ],
    },
    {
      title: "Autonomous",
      link: "/arc/autonomous",
      icon: <Map />,
      children: [
        {
          title: "Cameras",
          link: "/cameras",
        },
        {
          title: "Status Monitor",
          link: "/status-monitor",
        },
      ],
    },
  ],
  // arc: {
  //   "/arc": <House />,
  //   "/arc/post-landing": <RocketTakeoff />,
  //   "/arc/space-resources": <BoxSeam />,
  //   "/arc/excavation-construction": <ConeStriped />,
  //   "/arc/mapping-autonomous": <Map />,
  // },
  // urc: {
  //   "/urc": <House />,
  //   "/urc/science": <GraphUp />,
  //   "/urc/delivery": <Envelope />,
  //   "/urc/equipment-servicing": <WrenchAdjustable />,
  //   "/urc/autonomous-navigation": <Map />,
  // },
};

const Sidebar: React.FC = () => {
  const location = useLocation();
  const currentPath = location.pathname;
  const routePrefix = currentPath.split("/")[1];

  const uiState = useSelector((state: RootState) => state.uiState);

  const [expandedItems, setExpandedItems] = useState<string[]>([]);

  const handleItemClick = (link: string) => {
    if (expandedItems.includes(link)) {
      setExpandedItems(expandedItems.filter(item => item !== link));
    } else {
      setExpandedItems([...expandedItems, link]);
    }
  };

  return (
    <div className={`sidebar ${uiState.sidebarIsVisible ? 'visible' : 'hidden'}`}>
      <ul className="sidebar-items">
        {sidebarData.arc.map((val, key) => {
          const isExpanded = expandedItems.includes(val.link);
          return (
            <li
              key={key}
              className={`sidebar-item ${routePrefix === val.link ? "active" : ""} ${isExpanded ? "clicked" : ""}`}
              id={routePrefix === val.link ? "active" : ""}
              onClick={() => handleItemClick(val.link)}
            >
              <div className="sidebar-item-content">
                <div className="icon">{val.icon}</div>
                <div>{val.title}</div>
              </div>

              {isExpanded && (
                <ul className="sidebar-children">
                  {val.children.map((child, index) => (
                    <li key={index}>
                      <div>{child.title}</div>
                    </li>
                  ))}
                </ul>
              )}
            </li>
          );
        })}
      </ul>
    </div>
  );
};

export default Sidebar;

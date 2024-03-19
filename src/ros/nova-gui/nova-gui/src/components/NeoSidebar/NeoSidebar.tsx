import {
  Button,
  ModalBody,
  ModalContent,
  ModalHeader,
  ScrollShadow,
} from "@nextui-org/react";
import { SidebarWrapper } from "./SidebarWrapper";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { useUIActions } from "../../redux/actions/useUIActions";

import novaLogo from "../../assets/nova-logo.png";
import { Aperture, Camera, Home, Image } from "react-feather";
import { ReactNode } from "react";
import { useNavigate } from "react-router-dom";

interface SidebarEntry {
  title: string;
  route: string;
  icon: ReactNode;
}

interface SidebarInterface {
  [task: string]: SidebarEntry[];
}

const sidebarData: SidebarInterface = {
  ["Post Landing"]: [
    {
      title: "Dashboard",
      route: "/arc/post-landing",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/post-landing",
      icon: <Camera />,
    },
  ],
  ["Space Resources"]: [
    {
      title: " NIR Spectroscopy",
      route: "/arc/cameras/space-resources",
      icon: <Aperture />,
    },
    {
      title: "Microscope Gallery",
      route: "/arc/post-landing",
      icon: <Image />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/space-resources",
      icon: <Camera />,
    },
  ],
  ["Excavation and Construction"]: [
    {
      title: "Dashboard",
      route: "/arc/excavation-construction",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/excavation-construction",
      icon: <Camera />,
    },
  ],
  ["Autonomous"]: [
    {
      title: "Cameras",
      route: "/arc/cameras/autonomous",
      icon: <Camera />,
    },
  ],
};

export const NeoSidebar = () => {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const navigate = useNavigate();

  return (
    <SidebarWrapper
      isOpen={uiState.sidebarIsVisible}
      onClose={() => uiActions.setSideBarVisibility(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-row justify-start pt-12">
          <img src={novaLogo} className="w-24" alt="Nova Logo" />
        </ModalHeader>
        <ModalBody>
          <ScrollShadow>
            {Object.keys(sidebarData).map((item) => {
              return (
                <div className="flex flex-col pb-5">
                  <div className="text-sm font-light text-gray-400">{item}</div>
                  <div className="flex flex-col gap-2 mt-2">
                    {sidebarData[item].map((mode) => {
                      return (
                        <Button
                          onClick={() => navigate(mode.route)}
                          size="md"
                          variant="light"
                          fullWidth
                          className="pl-3"
                        >
                          <div className=" w-full flex flex-row justify-start gap-3 items-center m-0">
                            <div>{mode.icon}</div>
                            <div>{mode.title}</div>
                          </div>
                        </Button>
                      );
                    })}
                  </div>
                </div>
              );
            })}
          </ScrollShadow>
        </ModalBody>
      </ModalContent>
    </SidebarWrapper>
  );
};

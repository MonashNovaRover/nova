import {
  Button,
  ModalBody,
  ModalContent,
  ModalHeader,
  ScrollShadow,
} from "@nextui-org/react";
import { SidebarWrapper } from "./SidebarWrapper.tsx";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import novaLogo from "../../../assets/nova-logo.png";
import { useLocation, useNavigate } from "react-router-dom";
import { arcNavigationData, compNavigationData, generalNavigationData, testNavigationData, urcNavigationData } from "../../../utils/NavigationRoutes.tsx";

export const NeoSidebar = () => {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const navigate = useNavigate();
  const location = useLocation();

  const currentPath = location.pathname;

  const sidebarData =
      location.pathname === "/" ? compNavigationData :
      location.pathname.startsWith("/arc") ? arcNavigationData :
      location.pathname.startsWith("/urc") ? urcNavigationData :
      location.pathname.startsWith("/general") ? generalNavigationData :
      location.pathname.startsWith("/test") ? testNavigationData :
      {};

  return (
    <SidebarWrapper
      isOpen={uiState.sidebarIsVisible}
      onClose={() => uiActions.setSideBarVisibility(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-row justify-start pt-5">
          <img src={novaLogo} className="w-24" alt="Nova Logo" />
        </ModalHeader>
        <ModalBody>
          <ScrollShadow>
            {Object.keys(sidebarData).map((item) => {
              return (
                <div key={item} className="flex flex-col pb-5">
                  <div className="text-sm font-light">{item}</div>
                  <div className="flex flex-col gap-2 mt-2">
                    {sidebarData[item].map((mode) => {
                      const isCurrentSelected = currentPath === mode.route;
                      return (
                        <Button
                          onPress={() => navigate(mode.route)}
                          size="md"
                          variant={isCurrentSelected ? "solid" : "light"}
                          color={isCurrentSelected ? "primary" : "default"}
                          fullWidth
                          className="pl-3"
                          key={mode.route}
                        >
                          <div
                            className={`w-full flex flex-row justify-start gap-3 items-center m-0 ${
                              !isCurrentSelected && "text-gray-400"
                            }`}
                          >
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

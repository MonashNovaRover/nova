import { Button } from "@nextui-org/react";
import { arcNavigationData, compNavigationData, generalNavigationData, testNavigationData, urcNavigationData } from "../../../utils/NavigationRoutes";
import { useLocation, useNavigate } from "react-router-dom";

export const HomePage = () => {
  const location = useLocation();
  const navigate = useNavigate();

  const currentPath = location.pathname;
  
  const navigationData =
    currentPath === "/" ? compNavigationData :
    currentPath === "/arc" ? arcNavigationData :
    currentPath === "/urc" ? urcNavigationData :
    currentPath === "/general" ? generalNavigationData :
    currentPath === "/test" ? testNavigationData :
    {};


  return (
    <div>
      {Object.keys(navigationData).map((item) => {
        return (
          <div key={item} className="flex flex-col pb-5">
            <div className="text-sm font-light">{item}</div>
            <div className="flex flex-col gap-2 mt-2">
              {navigationData[item].map((mode) => {
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
    </div>
  );
}
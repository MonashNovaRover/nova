import { arcNavigationData, compNavigationData, generalNavigationData, testNavigationData, urcNavigationData } from "./NavigationRoutes";
import { useLocation } from "react-router-dom";

export const HomePage = () => {
  const location = useLocation();

  const currentPath = location.pathname;
  
  const navigationData =
    currentPath === "/" ? compNavigationData :
    currentPath === "/arc" ? arcNavigationData :
    currentPath=== "/urc" ? urcNavigationData :
    currentPath === "/general" ? generalNavigationData :
    currentPath === "/test" ? testNavigationData :
    {};


  return (
    <p>Home Page</p>
  );
}
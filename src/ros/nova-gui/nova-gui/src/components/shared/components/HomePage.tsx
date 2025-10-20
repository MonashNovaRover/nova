import { arcNavigationData, generalNavigationData, urcNavigationData } from "./NavigationRoutes";
import { useLocation } from "react-router-dom";

export const HomePage = () => {
  const location = useLocation();

  const currentPath = location.pathname;
  
  const navigationData =
    // TODO: Make compNavigationData and testNavigationData   
    // currentPath === "/" ? arcNavigationData :
    currentPath === "/arc" ? arcNavigationData :
    currentPath=== "/urc" ? urcNavigationData :
    currentPath === "/general" ? generalNavigationData :
    // currentPath === "/test" ? generalNavigationData :
    {};


  return (
    <p>Home Page</p>
  );
}
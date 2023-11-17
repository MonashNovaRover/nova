import { Outlet, useNavigate } from "react-router-dom";
import { NextUIProvider } from "@nextui-org/react";
import { NovaNavbar } from "./components/navbar/Navbar";
import configureRootStore from "./redux/store/configureRootStore";
import { Provider } from "react-redux";
import { SettingsModal } from "./components/settings/SettingsModal";

export const Root = () => {
  const navigate = useNavigate();
  const store = configureRootStore();

  return (
    <NextUIProvider navigate={navigate}>
      <Provider store={store}>
        <div className="dark text-foreground  w-screen h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
          <NovaNavbar />
          <SettingsModal />
          <Outlet />
        </div>
      </Provider>
    </NextUIProvider>
  );
};

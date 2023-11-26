import { Outlet, useNavigate } from "react-router-dom";
import { NextUIProvider } from "@nextui-org/react";
import { NovaNavbar } from "./components/navbar/navbar";

export const Root = () => {
  const navigate = useNavigate();

  return (
    <NextUIProvider navigate={navigate}>
      <div className="dark text-foreground  w-screen h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
        <NovaNavbar />
        <Outlet />
      </div>
    </NextUIProvider>
  );
};

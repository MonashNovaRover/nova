import { useNavigate } from "react-router-dom";
import { NextUIProvider } from "@nextui-org/react";
import { RosRoot } from "./RosRoot";

export const Root = () => {
  const navigate = useNavigate();

  return (
    <NextUIProvider navigate={navigate}>
      <RosRoot/>
    </NextUIProvider>
  );
};

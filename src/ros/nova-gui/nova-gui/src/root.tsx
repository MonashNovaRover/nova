import { useNavigate } from "react-router-dom";
import { HeroUIProvider } from "@heroui/react";
import { RosRoot } from "./RosRoot";

export const Root = () => {
  const navigate = useNavigate();

  return (
    <HeroUIProvider navigate={navigate}>
      <RosRoot/>
    </HeroUIProvider>
  );
};

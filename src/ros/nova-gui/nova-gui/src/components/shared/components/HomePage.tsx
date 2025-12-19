import { Button, Card } from "@nextui-org/react";
import { useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const navigate = useNavigate();

  const navigationData = props.navigationData

  return (
    <div className="flex flex-col items-center h-[calc(100vh-5rem)] gap-10 py-5 px-10">
      {Object.keys(navigationData).map((item) => {
        return (
          <Card key={item} fullWidth className="flex-1 p-2">
            <div className="text-lg font-semibold text-white px-3">{item}</div>
            <div className="flex gap-10 p-3 px-5 h-full">
              {navigationData[item].map((mode) => {
                return (
                  <Button
                    onPress={() => navigate(mode.route)}
                    size="lg"
                    variant="ghost"
                    color="default"
                    className="flex-1 h-full transition-transform duration-300 ease-in-out hover:scale-105"
                    key={mode.route}
                    startContent={mode.icon}
                  >
                      {mode.title}
                  </Button>
                );
              })}
            </div>
          </Card>
        );
      })}
    </div>
  );
}
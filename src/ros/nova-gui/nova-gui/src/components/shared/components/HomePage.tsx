import { Button, Card, CardBody, CardHeader } from "@nextui-org/react";
import { useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const navigate = useNavigate();

  const navigationData = props.navigationData

  return (
    <div className="flex flex-col items-center h-[calc(100vh-6rem)] gap-3">
      {Object.keys(navigationData).map((item) => {
        return (
          <Card key={item} fullWidth className="flex-1">
            <CardHeader className="text-lg font-semibold text-white">
              {item}
            </CardHeader>
            <CardBody>
              <div className="flex gap-3 p-0 h-full">
                {navigationData[item].map((mode) => {
                  return (
                    <Button
                      onPress={() => navigate(mode.route)}
                      size="lg"
                      variant="ghost"
                      color="default"
                      className="flex-1 h-full gap-3"
                      key={mode.route}
                      startContent={mode.icon}
                    >
                      {mode.title}
                    </Button>
                  );
                })}
              </div>
            </CardBody>
          </Card>
        );
      })}
    </div>
  );
}
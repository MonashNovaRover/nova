import { Button, Card, CardBody, CardHeader, Divider } from "@nextui-org/react";
import { useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const navigate = useNavigate();

  const navigationData = props.navigationData

  return (
    <div className="flex flex-col items-center h-screen gap-10 p-3 pt-15 pb-20">
      {Object.keys(navigationData).map((item) => {
        return (
          <Card key={item} className="w-[90%] flex-1">
            <CardHeader className="pl-4">
              <div className="text-lg font-semibold text-white">{item}</div>
            </CardHeader>
            <CardBody>
              <div className="flex gap-10 p-5 h-full">
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
            </CardBody>
          </Card>
        );
      })}
    </div>
  );
}
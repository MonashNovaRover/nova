import { Button, Card, CardBody, CardHeader, Divider } from "@nextui-org/react";
import { useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const navigate = useNavigate();

  const navigationData = props.navigationData

  return (
    <div className="flex flex-wrap justify-center items-center h-screen gap-10 p-3">
      {Object.keys(navigationData).map((item) => {
        return (
          <Card key={item} className="w-[20%]">
            <CardHeader className="flex justify-center">
              <div className="text-lg font-semibold text-white">{item}</div>
            </CardHeader>
            <Divider />
            <CardBody>
              <div className="flex flex-col gap-2 p-3">
                {navigationData[item].map((mode) => {
                  return (
                    <Button
                      onPress={() => navigate(mode.route)}
                      size="md"
                      variant="ghost"
                      color="default"
                      className="transition-transform duration-300 ease-in-out hover:scale-105"
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
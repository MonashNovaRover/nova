import { Button, Card, CardBody, CardHeader, Divider } from "@nextui-org/react";
import { useLocation, useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const location = useLocation();
  const navigate = useNavigate();

  const navigationData = props.navigationData

  const currentPath = location.pathname;

  return (
    <div className="flex justify-evenly items-center h-screen">
      {Object.keys(navigationData).map((item) => {
        return (
          // TEMP: className values setting width and height in card are temporary!!
          <Card key={item} className="w-[400px] h-[400px]">
            <CardHeader className="flex gap-3">
              <div className="text-sm font-light">{item}</div>
            </CardHeader>
            <Divider />
            <CardBody>
              <div className="flex flex-col gap-2 mt-2">
                {navigationData[item].map((mode) => {
                  const isCurrentSelected = currentPath === mode.route;
                  return (
                    <Button
                      onPress={() => navigate(mode.route)}
                      size="md"
                      variant={isCurrentSelected ? "solid" : "light"}
                      color={isCurrentSelected ? "primary" : "default"}
                      fullWidth
                      className="pl-3"
                      key={mode.route}
                    >
                      <div
                        className={`w-full flex flex-row justify-start gap-3 items-center m-0 ${
                          !isCurrentSelected && "text-gray-400"
                        }`}
                      >
                        <div>{mode.icon}</div>
                        <div>{mode.title}</div>
                      </div>
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
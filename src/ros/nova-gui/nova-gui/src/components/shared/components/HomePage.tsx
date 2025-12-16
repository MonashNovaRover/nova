import { Button, Card, CardBody, CardHeader, Divider } from "@nextui-org/react";
import { useLocation, useNavigate } from "react-router-dom";
import { HomePageProps } from "../../../views/shared/HomePageView";

export const HomePage = (props: HomePageProps) => {
  const location = useLocation();
  const navigate = useNavigate();

  const navigationData = props.navigationData

  return (
    <div className="flex justify-center items-center h-screen gap-5">
      {Object.keys(navigationData).map((item) => {
        return (
          // TEMP: className values setting width and height in card are temporary!!
          <Card key={item} className="w-[400px] h-[400px]">
            <CardHeader className="flex gap-3 justify-center">
              <div className="text-sm font-light">{item}</div>
            </CardHeader>
            <Divider />
            <CardBody>
              <div className="flex flex-col gap-2 mt-2">
                {navigationData[item].map((mode) => {
                  return (
                    <Button
                      onPress={() => navigate(mode.route)}
                      size="md"
                      variant="bordered"
                      color="default"
                      fullWidth
                      className="pl-3"
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
import { HomePage } from "../../components/shared/components/HomePage";
import { NavigationInterface } from "../../utils/NavigationRoutes";

export interface HomePageProps {
  navigationData: NavigationInterface;
}

const HomePageView = (props: HomePageProps) => {
  return (
    <HomePage navigationData={props.navigationData} />
  );
};

export default HomePageView;

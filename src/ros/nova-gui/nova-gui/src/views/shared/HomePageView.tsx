import { HomePage } from "../../components/shared/components/HomePage";
import { NavigationInterface } from "../../utils/NavigationRoutes";

export interface HomePageProps {
  navigationData: NavigationInterface;
}

const HomePageView = (props: HomePageProps) => {
  return (
    <div className="p-3">
      <HomePage navigationData={props.navigationData} />
    </div>
  );
};

export default HomePageView;

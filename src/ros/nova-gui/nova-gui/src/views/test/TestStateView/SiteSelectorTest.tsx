import {Card, CardBody} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SiteTypeSelectWidget from "../../../components/SiteSelectWidget/SiteTypeSelectWidget.tsx";

export default function SiteSelectorTest() {

  const [currentSite, _] = useGenericStore<Site>("currentSite");

  return (
    <Card>
      <CardBody>
        <SiteTypeSelectWidget/>
      </CardBody>
      <CardBody className="text-center">
        {currentSite}
      </CardBody>
    </Card>
  )
}



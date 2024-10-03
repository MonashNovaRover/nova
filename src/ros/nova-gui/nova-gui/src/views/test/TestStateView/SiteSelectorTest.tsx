import SiteSelectWidget, {siteFilenames} from "./SiteSelectWidget.tsx";
import {Card, CardBody} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";


export default function SiteSelectorTest() {

  const [currentSite, _] = useGenericStore<CurrentSiteStore>("currentSite");

  return (
    <Card>
      <CardBody>
        <SiteSelectWidget
          hideSiteType={false}
        />
      </CardBody>
      <CardBody className="text-center">
        {siteFilenames[currentSite.site]}
      </CardBody>
    </Card>
  )
}



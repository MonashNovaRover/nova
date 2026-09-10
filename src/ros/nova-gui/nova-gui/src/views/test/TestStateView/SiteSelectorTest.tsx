import {Card, CardContent} from "@heroui/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SiteTypeSelectWidget from "../../../components/science/SiteSelectWidget/SiteTypeSelectWidget.tsx";

export default function SiteSelectorTest() {

  const [currentSite, _] = useGenericStore<Site>("currentSite");

  return (
    <Card>
      <CardContent>
        <SiteTypeSelectWidget/>
      </CardContent>
      <CardContent className="text-center">
        {`site ${currentSite.valueOf() + 1}`}
      </CardContent>
    </Card>
  )
}



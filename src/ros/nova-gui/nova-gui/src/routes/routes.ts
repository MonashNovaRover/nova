import ARCEscavationConstructionView from "../views/arc/ARCEscavationConstructionView"
import ARCMappingAutonomousView from "../views/arc/ARCMappingAutonomous"
import ARCPostLandingView from "../views/arc/ARCPostLandingView"
import ARCSpaceResourcesView from "../views/arc/ARCSpaceResourcesView"
import ARCBaseView from "../views/base/ARCBaseView"
import GeneralBaseView from "../views/base/GeneralBaseView"
import HomeView from "../views/base/HomeView"
import URCBaseView from "../views/base/URCBaseView"
import URCAutonomousNavigationView from "../views/urc/URCAutonomousNavigationView"
import URCDeliveryView from "../views/urc/URCDeliveryView"
import URCEquipmentServicingView from "../views/urc/URCEquipmentServicingView"
import URCScienceView from "../views/urc/URCScienceView"


export type View = {
    name: string
    path: string
    component: React.FC
    icon?: React.FC
}

export const mainRoutes: View[] = [
    {
        name: "Home",
        path: "/",
        component: HomeView,
    },
    {
        name: "ARC",
        path: "/arc",
        component: ARCBaseView,
    },
    {
        name: "URC",
        path: "/urc",
        component: URCBaseView,
    },
    {
        name: "General",
        path: "/general",
        component: GeneralBaseView,
    }
]


export const arcRoutes: View[] = [
    {
        name: "Post Landing Task",
        path: "/arc/post-landing",
        component: ARCPostLandingView,
    },
    {
        name: "Space Resources Task",
        path: "/arc/space-resources",
        component: ARCSpaceResourcesView,
    },
    {
        name: "Escavation & Construction Task",
        path: "/arc/escavation-construction",
        component: ARCEscavationConstructionView,
    },
    {
        name: "Mapping & Autonomous Task",
        path: "/arc/mapping-autonomous",
        component: ARCMappingAutonomousView,
    },
]

export const urcRoutes: View[] = [
    {
        name: "Science Mission",
        path: "/urc/science",
        component: URCScienceView,
    },
    {
        name: "Delivery Mission",
        path: "/urc/delivery",
        component: URCDeliveryView,
    },
    {
        name: "Equipment Servicing Mission",
        path: "/urc/equipment-servicing",
        component: URCEquipmentServicingView,
    },
    {
        name: "Autonomous Navigation Mission",
        path: "/urc/autonomous-navigation",
        component: URCAutonomousNavigationView,
    },
]


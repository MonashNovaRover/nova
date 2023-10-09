import { View } from "./routes"
import { Route } from "react-router-dom"



const RoutesConstructor = (views : View[]) => {
    
    return (
        <>
            {
                views.map((view, index) => {
                    return (
                        <Route key={index} path={view.path} element={<view.component />} />
                    )
                })

            }
       </>
    )
}
export default RoutesConstructor

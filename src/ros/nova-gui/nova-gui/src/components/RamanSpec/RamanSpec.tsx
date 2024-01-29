/**
 * Author: Connor Macdougall
 * This component acts as a container for the three distinct components.
 * Use for simple layout configuration changes, and exporting.
 */

import RamanCCDInputs from "./RamanCCDInputs";
import RamanMechanicalInputs from "./RamanMechanicalInputs";
import RamanOutputComparison from "./RamanOutputComparison";

const RamanSpec: React.FC = () => {
    return (
        <div>
            <RamanMechanicalInputs />
            <RamanOutputComparison />
            <RamanCCDInputs />
        </div>
    )
}

export default RamanSpec;
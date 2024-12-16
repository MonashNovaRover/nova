interface SubCardLabelProps {
    children: React.ReactNode;
}

export const SubCardLabel : React.FC<SubCardLabelProps> = (props) => {

    return (
        <span className="text-sm uppercase tracking-widest text-center text-default-900 font-semibold text-opacity-80">
            {props.children}
        </span>
    );
};


import logo from "../../assets/logoEquipo15.jpeg";

const LINKS = []; //const LINKS = ["En vivo", "Gráficas", "Aterrizaje", "Paquetes"];

export default function Navbar() {
    return (
        <nav className="navbar navbar-expand bg-white shadow-sm px-3">
            <img src={logo} alt="Logo del Equipo 15" height="36" className="me-2" />
            <span className="navbar-brand fw-bold">Dashboard CanSat Equipo #15</span>
            <ul className="navbar-nav ms-auto">
                {LINKS.map((name, i) => (
                    <li className="nav-item" key={name}>
                        <a className={`nav-link${i === 0 ? " active" : ""}`} href="#">
                            {name}
                        </a>
                    </li>
                ))}
            </ul>
        </nav>
    );
}
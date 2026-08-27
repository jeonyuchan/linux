from dataclasses import dataclass

@dataclass
class Human:
    family_name: str = "Jeon"
    first_name: str = "Yuchan"
    born: int = 1995
    birth: str = "Jan. 3"

@dataclass
class Address:
    zipcode: str = "23467"
    country: str = "Republic of Korea"
    city: str = "Incheon" 
    address_line: str = "446, Geomdan-ro, Geomdan-gu"
    transference: str = "2007-07-16"
  
human = Human()
print(f"{human.first_name} {human.family_name}, born on {human.birth}, {human.born}")

address = Address()
formatted_date = address.transference.replace("-", ". ") + "."
print(f"{address.address_line}, {address.city}, {address.country} ({address.zipcode}) [relocated on {formatted_date}]")

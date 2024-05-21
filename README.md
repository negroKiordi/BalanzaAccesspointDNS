# BalanzaAccesspointDNS
Se implementa un accesspoint y un servidor en la dirección http://balanza.local
La MCU realiza la lectura de las caravanas inalámbricas y el peso mediante una celda de carga.


# Versión 1.0 implementa una API de solo texto sin pagina web.

Se puede consultar lo siguiente en "http://balanza.local"

/caravananew     si hay una nueva lectura de caravana

/caravanadata    el valor de la caravana caravana. Al acceder a esta dirección se resetea el estado de de "caravananew"

/pesoestable     el valor del último peso estable leido

/peso            el valor instantaneo de peso

/calibrarpeso?VALUE=  enviar como prámetro "VALUE" un float con el peso utilizado para la calibración

/tarar          tara la balanza


# Optimiseur-Photovoltaique
Evolution de l'optimiseur EcoPV
Cette version offre des evolutions importantes:
Au niveau de l'alimentation electrique:
  - une protection aux surtenssions et un filtrage sont incluses pour satisfaire aux normes de securité
  - deux sources d'alimentation
    - une utilisant un transformateur ( max 250mA) qui peut etre de 3.3 ou de 5V
    - une à découpage de 2 W: 3.3 600mA ou 5V 400 mA
On integre une interface Ethernet W5500 controle par bus SPI
On integre une interface WiFi avec un ESP-01S (et les libraries ESP-Link (https://github.com/jeelabs/esp-link)
On integre un capteur de temperature DS18B20 pour la gestion automatique des appoints en hiver ou en absence de soleil
On integre une detection des HC par lecture du contacteur du Linky afin de faire les appoints pendant les heures creuses.
On integre un switch vers un deuxieme cumulus ( ou un systeme de batteries) au cas ou le cumulus arrive à sa temperature maximale
On integre les circuits de mesure du courant et de la tension
Pour la mesure de la tension et la detection du crossover deux sorties tension centrees sur VCC/2 (donc 1.65 ou 2,5V) et sur OV sont ofertes
Un Ecran Oled 0,96 ". 
Le tout en format DIN 3 modules dans un boitier Z 107 de chez KRADEX 

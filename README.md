# TACOMETRO USB
## Tacômetro USB para jogos usando PIC12F1822.

#### Placa
![Placa da Placa](https://github.com/lucas458/TACOMETRO_USB/blob/main/IMAGENS/pcb.png?raw=true "Frente da placa")


### Dados para ser enviados, são enviados 6 bytes pela porta serial.
| Buffer Index | Valor (decimal) | Descrição |
| ------------ | --------------- | --------- |
| 0 | 0 a 255 | Digito 0 |
| 1 | 0 a 255 | Digito 1 |
| 2 | 0 a 255 | Digito 2 |
| 3 | 0 a 20 | Quantidade de LEDS ligados |
| 4 | 0 a 15 | Brilho |
| 5 | '\r' ou '\n' | **Caractere obrigatório** |

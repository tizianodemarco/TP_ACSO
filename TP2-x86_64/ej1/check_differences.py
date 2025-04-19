# Este script fue creado para comparar las salidas de la catedra y la propia, 
# dado que al correrlo localmente saltaron discrepancias inexistentes.

path1 = 'salida.caso.propio.ej1.txt'
path2 = 'salida.catedra.ej1.txt'

def leer_archivo(path):
    with open(path, 'r') as file:
        contenido = file.readlines()
    return contenido

def comparar_archivos(contenido1, contenido2):
    if len(contenido1) != len(contenido2):
        return False
    for i in range(len(contenido1)):
        if contenido1[i] != contenido2[i]:
            return False
    return True

contenido1 = leer_archivo(path1)
contenido2 = leer_archivo(path2)
if comparar_archivos(contenido1, contenido2):
    print("Los archivos son iguales")
else:
    print("Los archivos son diferentes")
    # Imprimir las diferencias
    for i in range(len(contenido1)):
        if contenido1[i] != contenido2[i]:
            print(f"Diferencia en la línea {i + 1}:")
            print(f"Archivo 1: {contenido1[i]}")
            print(f"Archivo 2: {contenido2[i]}")
            
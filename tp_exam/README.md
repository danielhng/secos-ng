DISCLAIMER

Notre travail n'est malheureusement pas entierement fonctionnel. 
Cela est du a une non réussite de notre part.
En effet l'appel de iret dans notre clock raise une page fault. C'est malheureux et nous n'avons pas trouver le moyen de résoudre le problème, à part de le localiser probablement dans l'execution de dépilation de la stack de la tache courante. 

Normalement, si nous avions résolus ce problème, l'execution du programme aurait entrainé une première éxecution de la tache 1 donc d'incrémentation, puis de read ect grace a la fonction clock.

CARTOGHRAPHIE MEMOIRE

base de la pgd: 0x200000
base de la ptb: 0x201000

tache 1: pgd en 0x202000
         page kernel identity mappee en 0x203000 adresses 0x0 a 0x0400
         page user privee identity mappee en 0x204000 adresses 0x400 a 0x800
         page user partagee en 0x205000 adresses 0x800 a 0xc00

tache 2: pgd en 0x206000
         page kernel identity mappee en 0x207000 adresses 0x0 a 0x400
         page user privee identity mappee en 0x209000 adresses 0x800 a 0xc00
         page user partagee en 0x208000 adresses 0x400 a 0x800

adresse idt: 0x30dfa0



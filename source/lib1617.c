/*
 * lib1617.c
 *
 *  Created on: Jul 21, 2017
 *      Author: nicolasfarabegoli
 */

#include "lib1617.h"

NODO *sentinel = NULL; //Sentinella per le foglie e padre della radice

						//   A	 B	 C	 D	  E	  F	  G	  H	  I	  J  K  L   M   N   O
int letter_frequencies[] = { 81, 15, 28, 43, 128, 23, 20, 61, 71, 2, 1, 40, 24, 69, 76,
						//  P   Q  R   S   T   U   V   W   X  Y	  Z	 SPC NULL NL  '  EOF
							20, 1, 61, 64, 91, 28, 10, 24, 1, 20, 1, 130, 80, 80, 5, 80};


//PROTOTIPI FUNZIONI AUSILIARIE
int createSentinel();
unsigned short alphabeticalOrder(char *, char *);
void leftRotate(NODO **, NODO *);
void rightRotate(NODO **, NODO *);
void insertFixUp(NODO **, NODO **);
void insertRBT(NODO **, NODO *);
NODO * searchWord(NODO *, char *);
short readWordDef(FILE*, char *, char *, bool *);
void removeChar(char *, char, char);
void rb_delete(NODO **, NODO *);
NODO * treeSuccessor(NODO *, NODO *);
void rb_trasplant(NODO **, NODO *, NODO *);
void rb_deleteFixUp(NODO **, NODO *);
NODO * treeMinimum(NODO *);

HNode * build_huffman_tree();
HNode * allocates_node(int);
HNode * extract_smaller_one(HNode **);
void fill_table(unsigned int *, HNode *, unsigned int);
int compress_node(NODO *, FILE *, unsigned int *);
void compress_string(char *, FILE *, unsigned int *);
NODO* find_index_word(NODO *, int, int *);
int decompress_file(FILE *, NODO **, HNode *);
int search_in_node(NODO *, MSWNode *, char *);
int levenshtein(const char *, int, const char *, int);
unsigned char convert_accent(unsigned char);
int empties_dictionary(NODO **);

//FUNZIONI "BASE"

NODO *createFromFile(char * nameFile)
{
	unsigned short i = 0;
	unsigned char tmp;
	NODO* root = NULL;
	NODO* node = NULL;
	FILE* f = fopen(nameFile, "r");

	if (f == NULL)
		return NULL;

	createSentinel();

	while (true)
	{
		//Create node
		node = (NODO*)malloc(sizeof(NODO));
		if (node == NULL)
			return NULL;

		//Create word space
		node->word = (char*)malloc(sizeof(char) * MAX_WORD);
		if (node->word == NULL)
			return NULL;

		//Add record
		node->def = NULL;
		node->isBlack = false;
		node->left = NULL;
		node->right = NULL;
		node->parent = NULL;

		tmp = getc(f);
		//Controllo che il carattere letto sia una lettera (anche accentata)
		for (i = 0; ((tmp >= 65 && tmp <= 90) || (tmp >= 97 && tmp <= 122) || (tmp >= 192 && tmp <= 252)); i++) {
			if (tmp >= 192 && tmp <= 252)
				tmp = convert_accent(tmp);
			else
				tmp = tolower(tmp);

			node->word[i] = tmp;
			tmp = getc(f);
		}
		node->word[i] = '\0'; //Add string terminator
		//Check if the word is 2 char lenght
		if (strlen(node->word) < 2 || strlen(node->word) > MAX_WORD) {
			free(node); //Release node
		}
		else {
			insertRBT(&root, node); //Insert node in RBT
		}
		//Check if the file pointer is at the end
		if (tmp == 0xff)
			break; //exit from the loop (infinity)
	}

	fclose(f);
	return root;
}

void printDictionary(NODO * dictionary) {//stampa in-order
	if (dictionary != NULL && dictionary != sentinel) {
		printDictionary(dictionary->left);
		printf("\"%s\": ", dictionary->word);
		printf("[%s]\n", dictionary->def);
		printDictionary(dictionary->right);
	}
}

void printDictionaryFile(NODO * dictionary, FILE *f) { //Function for print on file the dictionary
	if (dictionary != NULL && dictionary != sentinel) {
		printDictionaryFile(dictionary->left, f);
		fprintf(f, "\"%s\": ", dictionary->word);
		fprintf(f, "[%s]\n", dictionary->def);
		printDictionaryFile(dictionary->right, f);
	}
}

int countWord(NODO *n) {
	if (n == NULL || n->word == NULL)						//CASO BASE (SENTINELLA)
		return 0;
	return countWord(n->left) + countWord(n->right) + 1;	//NUMERO PAROLE DEL RAMO SINISTRO + DESTRO + IL NODO
}

int insertWord(NODO ** dictionary, char * word) {
	if (strlen(word) < 2)
		return 1;

	NODO* node = (NODO*)malloc(sizeof(NODO));
	if (node == NULL)
		return 1;

	node->def = NULL;
	node->isBlack = false;
	node->left = NULL;
	node->parent = NULL;
	node->right = NULL;
	node->word = (char *)malloc(sizeof(char) * MAX_WORD);
	if (node->word == NULL)
		return 1;
	strncpy(node->word, word, MAX_WORD);

	insertRBT(dictionary, node);		//inserimento del nodo
	return 0;
}

int cancWord(NODO ** dictionary, char * word)
{
	//search node
	NODO* sWord = searchWord(*dictionary, word);
	if (sWord == NULL)
		return 1;
	//delete node
	rb_delete(dictionary, sWord);
	return 0;
}

char* getWordAt(NODO *n, int index){

	//Check if the number of node is < index
	if(countWord(n) < index)
		return NULL;

	int cont = 0;
	NODO* tmp = NULL;

	tmp = find_index_word(n, index, &cont);

	return tmp->word;

}

/*char *getWordAt(NODO *n, int index) {
	int counter = 0;
	if (n == NULL)
		return NULL;
	return find_index_word(n, index, &counter);		//richiamo una funzione ausiliaria
}*/

int insertDef(NODO * dictionary, char * word, char * def)
{
	NODO* sWord = searchWord(dictionary, word);
	if (sWord == NULL)
		return 1; //Word not found

	sWord->def = (char*)malloc(sizeof(char) * MAX_DEF);
	if (sWord->def == NULL)
		return 1;
	strncpy(sWord->def, def, MAX_DEF); //Copy the definition with safe string copy
	return 0; //All ok
}

char *searchDef(NODO * dictionary, char * word)
{
	//search the word
	NODO* sWord = searchWord(dictionary, word);
	if (sWord == NULL) //check if the word exist
		return NULL;

	return sWord->def; //return the definition
}

int saveDictionary(NODO * dictionary, char * fileOutput) {
	//check if the dictionary is empty
	if (dictionary == NULL)
		return -1;

	//Create file for write on it
	FILE* f = fopen(fileOutput, "w");
	//check if the open file was fine
	if (f == NULL)
		return -1;

	//Print on file the dictionary
	printDictionaryFile(dictionary, f);

	fclose(f); //Close file
	return 0;
}

NODO *importDictionary(char * fileInput) {

	//open file
	FILE* f = fopen(fileInput, "rb");
	if (f == NULL)
		return NULL;

	bool endFile = false; //for detect EOF
	NODO* root = NULL;
	NODO* node = NULL;

	//Allocate string for read from file
	char* word = (char*)malloc(sizeof(char) * MAX_WORD + 5);
	if (word == NULL)
		return NULL;
	char* def = (char*)malloc(sizeof(char) * MAX_DEF + 5);
	if (def == NULL)
		return NULL;
	//loop end of file
	while (!endFile) {
		//read word and definition
		if (readWordDef(f, word, def, &endFile) == -1)
			return NULL;
		//new node
		node = (NODO*)malloc(sizeof(NODO));
		if (node == NULL)
			return NULL;
		node->def = (char*)malloc(sizeof(char) * MAX_DEF);
		node->word = (char*)malloc(sizeof(char) * MAX_WORD);
		if (node->word == NULL || node->def == NULL)
			return NULL;
		//check if the def is null
		if (!strncmp(def, "(null)", MAX_DEF))
			node->def = NULL;
		else
			strncpy(node->def, def, MAX_DEF);
		strncpy(node->word, word, MAX_WORD);
		node->isBlack = false;
		node->left = NULL;
		node->parent = NULL;
		node->right = NULL;
		//add to the tree
		insertRBT(&root, node);
	}

	return root;
}

int searchAdvance(NODO * dictionary, char * word, char ** first, char ** second, char ** third) {
	if (word == NULL || dictionary == NULL) {
		*first = *second = *third = NULL;
		return -1;
	}

	MSWNode head[3];
	head[0].w_pointer = first;
	head[1].w_pointer = second;
	head[2].w_pointer = third;
	head[0].distance = head[1].distance = head[2].distance = 0x7fff;

	int res = search_in_node(dictionary, head, word);	//chiamo search in node
	if (head[2].distance == 0x7fff)						//se ho trovato solo 2 parole simili
		return -1;
	return res;
}

int compressHuffman(NODO * dictionary, char * file_name) {
	char eof = 27;
	unsigned int code_table[ELEMENTS];		//tabella delle codifiche
	FILE *output_file = fopen(file_name, "wb");
	HNode *root = build_huffman_tree();		//albero delle codifiche
	if (dictionary == NULL || root == NULL)
		return -1;
	fill_table(code_table, root, 0);		//salvo le codifiche nella tabella
	compress_node(dictionary, output_file, code_table);	//comprimo
	compress_string(&eof, output_file, code_table);
	fclose(output_file);

	return 0;
}

int decompressHuffman(char * file_name, NODO ** dictionary) {
	int x = 0;
	FILE *input_file = fopen(file_name, "rb");
	HNode *root = build_huffman_tree();
	empties_dictionary(dictionary);						//svuoto il dizionario attuale
	if (sentinel == NULL)
		x = createSentinel();
	(*dictionary) = sentinel;
	if (root == NULL || input_file == NULL || x == -1)
		return -1;
	x = decompress_file(input_file, dictionary, root);	//decomprimo il file
	fclose(input_file);

	return x;
}

//FUNZIONI AUSILIARIE

HNode *build_huffman_tree() {
	HNode *nodes_head = allocates_node(0);				//ALLOCO IL PRIMO NODO DELLA LISTA
	HNode *temp = nodes_head;

	if (temp == NULL)									//CONTROLLO L'ALLOCAZIONE
		return NULL;

	temp->frequencies = letter_frequencies[0];			//FREQUENZA DELLA LETTERA (CONTENUTA NELL'ARRAY)

	for (int i = 1; i < ELEMENTS; i++) {				//PER OGNI LETTERA DOPO LA PRIMA, ALLOCO UN NODO (CREO UNA LISTA)
		temp->next = allocates_node(i);					//ALLOCO UN NUOVO NODO E LO FACCIO PUNTARE DAL NEXT DEL PRECEDENTE
		temp = temp->next;								//SPOSTO LA VAR TEMP AL NODO APPENA CREATO (O A NULL IN CASO D'ERRORE)
		if (temp == NULL)								//CONTROLLO L'ALLOCAZIONE
			return NULL;
		temp->frequencies = letter_frequencies[i];		//ASSOCIO LA FREQUENZA DELLA LETTERA CONTENUTA NELL'ARRAY ALLA VARIABILE NEL NODO
	}

	while (nodes_head->next != NULL) {					//QUANDO HO ALTRI NODI OLTRE LA "RADICE" (SE NON CI SON STATI ERRORI PRECEDENTI CI SARA' SEMPRE ALMENO UN NODO; IN CASO CONTRARIO AVREBBE RESTITUITO NULL)
		temp = allocates_node(127);						//ALLOCO UN NODO CHE NON CORRISPONDE A NESSUNA LETTERA
		if (temp == NULL)								//CONTROLLO L'ALLOCAZIONE
			return NULL;
		temp->left = extract_smaller_one(&nodes_head);	//ESTRAGGO IL NODO CON LETTERA DI FREQUENZA MINORE DALLA LISTA E LO FACCIO PUNTARE DAL FIGLIO SINISTRO DEL NUOVO NODO
		temp->right = extract_smaller_one(&nodes_head);	//ESTRAGGO IL NODO CON LETTERA DI FREQUENZA MINORE DALLA LISTA E LO FACCIO PUNTARE DAL FIGLIO DESTRO DEL NUOVO NODO
		temp->frequencies = temp->left->frequencies + temp->right->frequencies;	//LA FREQUENZA EQUIVALE ALLA SOMMA DELLE DUE
		temp->next = nodes_head;						//INSERISCO IL NODO NELLA LISTA
		nodes_head = temp;
	}
	return nodes_head;									//RITORNO LA TESTA DELLA LISTA CHE ORA CORRISPONDERA' ALLA RADICE DELL'ALBERO DI HUFFMAN
}

HNode *allocates_node(int i) {
	HNode *node = (HNode *)malloc(sizeof(HNode));		//ALLOCO IL NODO
	if (node == NULL)									//CONTROLLO L'ALLOCAZIONE
		return NULL;

	node->letter = i;									//INDICE DELL'ARRAY CORRISPONDENTE ALLA LETTERA
	node->left = node->right = node->next = NULL;

	return node;										//PUNTATORE AL NODO
}

HNode *extract_smaller_one(HNode **nodes_head) {
	HNode *minimum = *nodes_head;
	HNode *scrolling = *nodes_head, *previous = NULL;

	if (*nodes_head == NULL)							//SE LA LISTA NON CONTIENE ELEMENTI
		return NULL;

	if ((*nodes_head)->next == NULL) {					//SE CONTIENE SOLO UN ELEMENTO
		*nodes_head = NULL;								//SVUOTO LA LISTA
		return minimum;									//RITORNO LA RADICE DELL'ALBERO
	}

	while (scrolling->next != NULL) {					//SCORRO LA LISTA
		if (scrolling->next->frequencies < minimum->frequencies) {	//SE TROVO UN ELEMENTO DI FREQUENZA MINORE
			previous = scrolling;						//TENGO IN MEMORIA IL NODO PRECEDENTE AL MINIMO
			minimum = scrolling->next;					//AGGIORNO IL MINIMO
		}
		scrolling = scrolling->next;					//VADO ALL'ELEMENTO SUCCESSIVO DELLA LISTA
	}
	if (previous == NULL) {								//SE L'ELEMENTO ERA IL PRIMO DELLA LISTA
		*nodes_head = minimum->next;					//SPOSTO LA TESTA ALL'ELEMENTO SUCCESSIVO
		minimum->next = NULL;							//SCOLLEGO IL PUNTATORE AL SUCCESSIVO DEL MINIMO
	}
	else {												//SE IL MINIMO NON ERA IL PRIMO DELLA LISTA
		previous->next = minimum->next;					//IL PUNTATORE NEXT DEL NODO PRECEDENTE PUNTA AL SUCCESSIVO DI MINIMO
		minimum->next = NULL;							//SCOLLEGO IL PUNTATORE NEXT DEL MINIMO
	}
	return minimum;										//RESTITUISCO L'INDIRIZZO DEL NODO CON FREQUENZA MINORE
}

void fill_table(unsigned int *code_table, HNode *tree_node, unsigned int code) {
	if (tree_node->letter != 127)									//SE SIAMO ALLA FOGLIA (C'E' UN VALORE != DA 127)
		code_table[(int)tree_node->letter] = code;					//"CODE" HA ASSUNTO IL VALORE "BINARIO" DEL PERCORSO DA RADICE->FOGLIA
	else {															//SE NON SONO ANCORA GIUNTO ALLA FOGLIA
		if ((code % 10) == 1 || (code % 10) == 2) {
			fill_table(code_table, tree_node->left, code + 3);		//SE VADO A SX IL RAMO HA VALORE 0(1) - IN QUESTO CASO E' STATO MESSO +3 PER EVITARE CHE L'INT RAGGIUNGESSE I 10 MILIARDI
			fill_table(code_table, tree_node->right, code + 5);		//SE VADO A DX IL RAMO HA VALORE 1(2) - IN QUESTO CASO E' STATO MESSO +5 PER EVITARE CHE L'INT RAGGIUNGESSE I 10 MILIARDI
		}
		else {
			fill_table(code_table, tree_node->left, code * 10 + 1);	//SE VADO A SX IL RAMO HA VALORE 0(1)
			fill_table(code_table, tree_node->right, code * 10 + 2);//SE VADO A DX IL RAMO HA VALORE 1(2)
		}
	}

	return;
}

int compress_node(NODO * n, FILE * output, unsigned int *code_table) {
	if (n == NULL || n->word == NULL)
		return -1;
	compress_node(n->left, output, code_table);			//compressione in order
	compress_string(n->word, output, code_table);
	compress_string(n->def, output, code_table);
	compress_node(n->right, output, code_table);

	return 0;
}

void compress_string(char *n_string, FILE *output, unsigned int *code_table) {
	char bit, c, byte = 0;										//1 BYTE
	unsigned int code, lenght, i = 0, bits_left = 8;

	do{
		if (n_string == NULL) {
			lenght = (int)log10((code_table[27]) + 1);			//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
			code = code_table[27];								//CODIFICA BIN
		}
		else {
			c = n_string[i];

			if (c >= 97 && c <= 122) {							//LETTERE MINUSCOLE
				lenght = (int)log10((code_table[c - 97]) + 1);	//LENGHT = LUNGHEZZA CODIFICA (CODIFICA ASCII DELLE LETTERE - 97 = INDICE LETTERA NELLA TABELLA)
				code = code_table[c - 97];						//CODIFICA "BINARIA" DELLA LETTERA
			}
			if (c == 32) {										//CARATTERE SPAZIO
				lenght = (int)log10((code_table[26]) + 1);		//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
				code = code_table[26];							//CODIFICA BIN
			}
			if (c == 0) {										//CARATTERE NULL
				lenght = (int)log10((code_table[27]) + 1);		//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
				code = code_table[27];							//CODIFICA BIN
			}
			if (c == 10) {										//CARATTERE "NEW LINE"
				lenght = (int)log10((code_table[28]) + 1);		//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
				code = code_table[28];							//CODIFICA BIN
			}
			if (c == 96) {										//CARATTERE "APOSTROFO"
				lenght = (int)log10((code_table[29]) + 1);		//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
				code = code_table[29];							//CODIFICA BIN
			}
			if (c == 27) {										//CARATTERE "ESCAPE"
				lenght = (int)log10((code_table[30]) + 1);		//LUNGHEZZA BINARIA DEL NUMERO CODIFICATO
				code = code_table[30];							//CODIFICA BIN
			}
		}
		while (lenght + 1 > 0)									//MI SCORRO TUTTI I "BIT" DELLA CODIFICA
		{
			bit = (code / pow(10, lenght)) - 1;					//PRENDO "BIT PER BIT" LA CODIFICA DELLA LETTERA (-1 PERCHE' GLI 0 = 1 E 1 = 2 NELLA TABELLA)
			switch (bit)
			{
			case 3:
				bit = 0;
				code -= (3 * pow(10, lenght));
				lenght++;
				break;
			case 4:
				bit = 1;
				code -= (4 * pow(10, lenght));
				lenght++;
				break;
			case 5:
				bit = 0;
				code -= (4 * pow(10, lenght));
				lenght++;
				break;
			case 6:
				bit = 1;
				code -= (5 * pow(10, lenght));
				lenght++;

				break;
			default:
				code -= (bit + 1) * pow(10, lenght);			//ELIMINO IL BIT VALUTATO
			}
			byte = byte | bit;									//PRENDO LA CODIFIA BINARIA DEL RESTO E LA METTO IN OR CON LE PRECEDENTI
			bits_left--;										//BIT NON "UTILIZZATI"
			lenght--;											//LUNGHEZZA DELLA CODIFICA RIMANENTE DA INSERIRE NEL FILE
			if (bits_left == 0) {								//SE HO RIEMPITO IL BYTE CON LA CODIFICA DELLE VARIE LETTERE
				fputc(byte, output);							//SCRIVO LA CODIFICA
				byte = 0;										//AZZERO X PER POTER "SCRIVERE" IL NUOVO BYTE
				bits_left = 8;									//RESETTO I BYTE "RIMANENTI" DI "BYTE"
			}
			byte = byte << 1;									//AVENDO INSERITO UNA CODIFICA "SHIFTO" I BIT PER NON SOVRASCRIVERLI
		}
		i++;
	} while (n_string != NULL && c != '\0');

	if (bits_left != 8){										//SE CODIFICANDO L'INTERA LINEA NON HO "RIEMPITO" IL BYTE
		byte = byte << (bits_left - 1);							//SHIFTO I BIT A SINISTA IN MODO DA NON AVERE "ZERI" CHE SI FRAPPONGANO FRA L'ULTIMA E LA PRECEDENTE LETTERA
		fputc(byte, output);									//SCRIVO IL BYTE NEL FILE
	}

	return;
}

/*char *find_index_word(NODO *n, int index, int *counter_pt) {
	if (n->word == NULL)										//CASO BASE (SENTINELLA)
		return NULL;

	char *result = find_index_word(n->left, index, counter_pt);	//CONTROLLO IL FIGLIO SINISTRO
	if (result != NULL)											//SE HO TROVATO LA PAROLA LA RESTITUISCO
		return result;

	if (*counter_pt == index)									//SE SONO ALLA I-ESIMA PAROLA (PARTENDO A CONTARE DA 1)
		return n->word;											//LA RESTITUISCO
	(*counter_pt)++;
	return find_index_word(n->right, index, counter_pt);		//ALTRIMENTI RESTITUISCO  QUELLO CHE MI PASSA IL FIGLIO DESTRO
}*/

NODO* find_index_word(NODO* n, int index, int *counter){



	find_index_word(n->left, index, counter);
	(*counter)++;
	if(index == *counter)
		return n;
	find_index_word(n->right, index, counter);


}

int decompress_file(FILE *input, NODO **dict_root, HNode *tree) {
	HNode *current = tree;
	register char c;
	register int k = 0;
	char bit;
	int i, end_of_file = 0;

	NODO *nodo_pointer = (NODO *)malloc(sizeof(NODO));
	char *string = (char *)malloc(sizeof(char) * 20);
	if (nodo_pointer == NULL || string == NULL)
		return -1;
	nodo_pointer->word = string;

	if ((c = fgetc(input)) == EOF)
		return -1;

	while (end_of_file != 1) {											//ACQUISISCE CICLICAMENTE I CARATTERI FINCHE' NON ARRIVA ALLA FINE DEL FILE
		for (i = 0; i<8; i++) {											//PER OGNI BIT
			bit = c & 0x80;												//PRENDE IL PRIMO BIT (AND CON 10000000)
			c = c << 1;													//SHIFTO IL BYTE
			if (bit == 0)												//SE IL BIT "ESTRATTO" E' "ZERO"
				current = current->left;								//MI SPOSTO SUL RAMO SINISTRO DELL'ALBERO
			else
				current = current->right;

			if (current->letter != 127){								//SE SONO GIUNTO AD UNA FOGLIA
				if (current->letter >= 0 && current->letter <= 25) {	//SE E' UNA LETTERA
					string[k] = current->letter + 97;
					k++;
				}
				if (current->letter == 26) {							//SE E' LO SPAZIO
					string[k] = 32;
					k++;
				}
				if (current->letter == 27) {							//SE E' NULL
					if (k != 0)
						string[k] = '\0';
					else {
						free(string);
						if (string == nodo_pointer->word)
							nodo_pointer->word = NULL;
						else
							nodo_pointer->def = NULL;
						string = NULL;
					}
					if (string == nodo_pointer->word) {
						string = (char *)malloc(sizeof(char) * 50);
						if (string == NULL)
							return -1;
						nodo_pointer->def = string;
					}
					else {
						insertRBT(dict_root, nodo_pointer);
						nodo_pointer = (NODO *)malloc(sizeof(NODO));
						if (nodo_pointer == NULL)
							return -1;
						string = (char *)malloc(sizeof(char) * 20);
						if (string == NULL)
							return -1;
						nodo_pointer->word = string;
					}
					if (string == NULL)
						return -1;
					k = 0;
					i = 7;
				}
				if (current->letter == 28) {							//SE E' NEW LINE
					string[k] = 10;
					k++;
				}
				if (current->letter == 29) {							//SE E' L'APOSTROFO
					string[k] = 96;
					k++;
				}														//"STAMPO" NEW LINE NEL FILE
				if (current->letter == ELEMENTS - 1) {					//SE E' L'EOF CREATO DA ME
					end_of_file = 1;
				}
				current = tree;											//TORNO ALLA RADICE
			}
		}
		c = fgetc(input);
	}
	return 0;
}

int search_in_node(NODO *n, MSWNode *head, char *word) {
	if (n->word == NULL)
		return 0;

	int ris = search_in_node(n->left, head, word);

	if (!strcmp(n->word, word))
		ris = 1;
	else {
		int ln = strlen(n->word);
		int lw = strlen(word);
		int dist = levenshtein(n->word, ln, word, lw);
		if (dist < head[2].distance) {		//se la nuova parola � pi� simile la sostituisco alla meno simile di quelle gia salvate
			head[2].distance = dist;
			*(head[2].w_pointer) = n->word;
			if (head[2].distance < head[1].distance) { //se � pi� simile della seconda ne cambio posizione
				MSWNode tmp = head[1];
				head[1] = head[2];
				head[2] = tmp;
				if (head[1].distance < head[0].distance) { //idem col precedente if
					tmp = head[0];
					head[0] = head[1];
					head[1] = tmp;
				}
			}
		}
	}
	return ris | search_in_node(n->right, head, word);
}

int levenshtein(const char *s, int ls, const char *t, int lt) {
	if (!ls)
		return lt;
	if (!lt)
		return ls;

	if (s[ls] == t[ls])
		return levenshtein(s, ls - 1, t, lt - 1);		//mi sposto sui vari caratteri delle parole

	int a = levenshtein(s, ls - 1, t, lt - 1);
	int b = levenshtein(s, ls, t, lt - 1);
	int c = levenshtein(s, ls - 1, t, lt);

	if (a > b) a = b;									//prendo la pi� simile
	if (a > c) a = c;

	return a + 1;
}

unsigned char convert_accent(unsigned char c) {
	if ((c >= 192 && c <= 198) || (c >= 224 && c <= 230))	//trasformo gli accenti in caratteri
		return 'a';
	if ((c >= 200 && c <= 203) || (c >= 232 && c <= 235))
		return 'e';
	if ((c >= 204 && c <= 207) || (c >= 236 && c <= 239))
		return 'i';
	if ((c >= 210 && c <= 214) || c == 216 || c == 240 || (c >= 242 && c <= 248))
		return 'o';
	if ((c >= 217 && c <= 220) || (c >= 249 && c <= 252))
		return 'u';
	if (c == 223)
		return 'b';
	if (c == 199 || c == 231)
		return 'c';
	if (c == 208)
		return 'd';
	if (c == 209 || c == 241)
		return 'n';
	if (c == 222)
		return 'p';
	if (c == 215)
		return 'x';
	if (c == 221)
		return 'y';

	return ' '; //NEVER HERE
}

int empties_dictionary(NODO **dictionary) {
	if ((*dictionary) == sentinel || (*dictionary) == NULL)
		return 0;
	NODO *l = (*dictionary)->left;				//salvo due sottoalberi (figli)
	NODO *r = (*dictionary)->right;
	free((*dictionary));						//elimino la radice
	empties_dictionary(&l);						//richiamo sui sottoalberi
	empties_dictionary(&r);
	return 0;
}

short readWordDef(FILE* f, char* word, char* def, bool* endFile) {

	char rchar; //simple sentinel for find EOF
	//Check if the argumets are omogeneous
	if (f == NULL || word == NULL || def == NULL)
		return -1; //bad

	fscanf(f, "%s", word); //read the line
	removeChar(word, '\"', ':');

	fscanf(f, "%[^\"]s", def); //read the line
	removeChar(def, '[', ']'); //remove [] character
	removeChar(def, '\n', ' ');
	removeChar(def, '\r', '\r');
	//detect the EOF
	if ((rchar = getc(f)) == EOF)
		*endFile = true;

	return 0; //ok

}

void removeChar(char* str, char garbage, char garbage2) {
	char *src, *dst; //to strings pointer
	for (src = dst = str; *src != '\0'; src++) {
		*dst = *src; //copy char form src to dst
		if (*dst != garbage && *dst != garbage2) dst++; //not find garbage character
	}
	*dst = '\0'; //ad end string
}

void rb_delete(NODO** root, NODO* z) {
	NODO* y = z;
	NODO* x;
	bool original = y->isBlack;
	//controllo se il figlio destro � la sentinella
	if (z->left == sentinel) {
		x = z->right;
		rb_trasplant(root, z, z->right);
	}
	//Controllo se il figlio sinsitro � sentinella
	else if (z->right == sentinel) {
		x = z->left;
		rb_trasplant(root, z, z->left);
	}
	//entrambi i figli sono nodi
	else {
		y = treeMinimum(z->right);
		original = y->isBlack;
		x = y->right;
		if (y->parent == z)
			x->parent = y;
		else {
			rb_trasplant(root, y, y->right);
			y->right = z->right;
			y->right->parent = y;
		}
		rb_trasplant(root, z, y);
		y->left = z->left;
		y->left->parent = y;
		y->isBlack = z->isBlack;
	}
	if (original == true)
		rb_deleteFixUp(root, x);

	free(z);
}

NODO* treeSuccessor(NODO* root, NODO* x) {
	if (x->right != sentinel)
		return treeMinimum(x->right); //ritono il minimo

	NODO* y = x->parent;
	//Scorro fino a che non trovo il successore
	while (y != NULL && x == y->right) {
		x = y;
		y = y->parent;
	}
	return y; //successore
}

void rb_trasplant(NODO** root, NODO* u, NODO* v) {
	//Sono nella radice
	if (u->parent == sentinel)
		*root = v;
	//figlio sinistro
	else if (u == u->parent->left)
		u->parent->left = v;
	else { //figlio destro
		u->parent->right = v;
	}

	v->parent = u->parent; //aggiorno puntatori
}

void rb_deleteFixUp(NODO** root, NODO * x) {

	NODO* w = NULL;
	//Scorro
	while (*root != x && x->isBlack == true) {
		//� a sinistra
		if (x == x->parent->left) {
			w = x->parent->right;
			if (w->isBlack == false) {
				w->isBlack = true;
				x->parent->isBlack = false;
				leftRotate(root, x->parent); //rotazione
				w = x->parent->right;
			}
			if ((w->left->isBlack == true) && (w->right->isBlack == true)) {
				w->isBlack = false;
				x = x->parent;
			}
			else {
				if (w->right->isBlack == true) {
					w->left->isBlack = true;
					w->isBlack = false;
					rightRotate(root, w); //rotazione destra
					w = x->parent->right; //nodo di sinistra
				}
				w->isBlack = x->parent->isBlack;
				x->parent->isBlack = true;
				w->right->isBlack = true;
				leftRotate(root, x->parent);
				x = *root; //assegno la radice
			}
		}
		else { //� a destra
			w = x->parent->left;
			if (w->isBlack == false) {
				w->isBlack = true;
				x->parent->isBlack = false;
				rightRotate(root, x->parent);
				w = x->parent->left;
			}
			if ((w->right->isBlack == true) && (w->left->isBlack == true)) {
				w->isBlack = false;
				x = x->parent; //aggiorno al padre
			}
			else {
				if (w->left->isBlack == true) {
					w->right->isBlack = true;
					w->isBlack = false;
					leftRotate(root, w); //rotazione sinistra
					w = x->parent->left;
				}
				w->isBlack = x->parent->isBlack;
				x->parent->isBlack = true;
				w->left->isBlack = true;
				rightRotate(root, x->parent); //rotazione destra
				x = *root; // assegno radice
			}
		}

	}
	x->isBlack = true; //sistemo colore
}

NODO* treeMinimum(NODO* x) {
	//Scorro fino al minimo
	while (x->left != sentinel)
		x = x->left;

	return x;
}

int createSentinel() {
	//Create node
	sentinel = (NODO*)malloc(sizeof(NODO));
	//Check for a safe malloc
	if (sentinel == NULL)
		return -1; //Bad malloc

	//Update record for sentinel
	sentinel->isBlack = true;
	sentinel->parent = NULL;
	sentinel->left = NULL;
	sentinel->right = NULL;
	sentinel->word = NULL;
	sentinel->def = NULL;

	return 0; //Ok
}

/*
*RETURN 0: n2 is grater than n1
*RETURN 1: n1 is grater than n2
*RETURN 2: n1 is n2
*RETURN 3: n1-> NULL or n2-> NULL
*/
unsigned short alphabeticalOrder(char* n1, char* n2) {

	if (n1 == NULL || n2 == NULL)
		return 3;
	//Check character
	for (int i = 0; i < MAX_WORD; i++) {
		if (n1[i] < n2[i])
			return 0;
		else if (n1[i] > n2[i])
			return 1;
	}

	return 2;
}

NODO* searchWord(NODO* root, char* word) {
	//caso base
	if (root == NULL || alphabeticalOrder(root->word, word) == 2)
		return root;

	switch (alphabeticalOrder(root->word, word)) {
	case 0: //Peso Maggiore
		return searchWord(root->right, word);
		break;
	case 1: //peso minore
		return searchWord(root->left, word);
		break;
	}

	//Here there is an error
	return NULL;

}

void leftRotate(NODO** root, NODO* x)
{
	NODO* y = x->right; //Create new NODO and assign to x.right
	x->right = y->left; //Move left sub-tree (y) on the x sub-tree

	if (y->left != sentinel) y->left->parent = x;

	y->parent = x->parent; //Connect parent of x to y

	if (x->parent == sentinel)
		*root = y;
	else if (x == x->parent->left)
		x->parent->left = y;

	else x->parent->right = y;

	y->left = x; //Move x on the left of y
	x->parent = y;
}

void rightRotate(NODO** root, NODO* y)
{
	NODO* x = y->left; //Create new NODO and assign to x.right
	y->left = x->right; //Move left sub-tree (y) on the x sub-tree

	if (x->right != sentinel) x->right->parent = y;

	x->parent = y->parent; //Connect parent of x to y

	if (y->parent == sentinel)
		*root = x;
	else if (y == y->parent->right)
		y->parent->right = x;

	else y->parent->left = x;

	x->right = y; //Move x on the left of y
	y->parent = x;
}

void insertFixUp(NODO** root, NODO** node) {
	NODO* T = *root;
	NODO* z = *node; //Puntatore temporaneo al nodo
	NODO* y = NULL;
	//Ciclo
	while (z->parent != NULL && z->parent->isBlack == false) {
		//Nodo di sinsitra
		if (z->parent == z->parent->parent->left) {
			y = z->parent->parent->left;
			if (y->isBlack == false) {
				z->parent->isBlack = true;
				y->isBlack = true;
				z->parent->parent->isBlack = false;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->right) {
					z = z->parent;
					leftRotate(root, z); //rotazione sinistra
				}
				z->parent->isBlack = true;
				z->parent->parent->isBlack = false;
				rightRotate(root, z->parent->parent);
			}
		}
		else { //nodo di destra
			y = z->parent->parent->left;
			if (y->isBlack == false) {
				z->parent->isBlack = true;
				y->isBlack = true;
				z->parent->parent->isBlack = false;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->left) {
					z = z->parent;
					rightRotate(root, z); //rotazione destra
				}
				z->parent->isBlack = true;
				z->parent->parent->isBlack = false;
				leftRotate(root, z->parent->parent);
			}
		}
	}

	T->isBlack = true;
}

void insertRBT(NODO** root, NODO* node) {
	NODO* y = NULL;
	NODO* x = NULL;

	y = sentinel;
	x = *root;
	//Scorro fino a che non sono finiti i nodi
	while (x != sentinel && x != NULL) {
		y = x;
		switch (alphabeticalOrder(node->word, x->word))	{
		case 0: //Caso maggiore
			x = x->left;
			break;
		case 1: //caso minore
			x = x->right;
			break;
		case 2:
			return;
		default:
			break;
		}
	}

	node->parent = y;
	if (y == sentinel)
		*root = node;
	else {
		switch (alphabeticalOrder(node->word, y->word))	{
		case 0:
			y->left = node;
			break;
		case 1:
			y->right = node;
			break;
		case 2:
			return;
		default:
			break;
		}
	}
	node->left = sentinel;
	node->right = sentinel;
	node->isBlack = false;
	insertFixUp(root, &node);
}

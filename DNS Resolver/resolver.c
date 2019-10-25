#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <errno.h>

/*TODO:
- MEMORY LEAKS
	- In the past I have had trouble with the ascii_to_name function, look there for possible memory leaks
*/

typedef unsigned int dns_rr_ttl;
typedef unsigned short dns_rr_type;
typedef unsigned short dns_rr_class;
typedef unsigned short dns_rdata_len;
typedef unsigned short dns_rr_count;
typedef unsigned short dns_query_id;
typedef unsigned short dns_flags;

typedef struct {
	char *name;
	dns_rr_type type;
	dns_rr_class class;
	dns_rr_ttl ttl;
	dns_rdata_len rdata_len;
	unsigned char *rdata;
} dns_rr;

struct dns_answer_entry;
struct dns_answer_entry {
	char *value;
	struct dns_answer_entry *next;
};
typedef struct dns_answer_entry dns_answer_entry;

void free_answer_entries(dns_answer_entry *ans) {
	dns_answer_entry *next;
	while (ans != NULL) {
		next = ans->next;
		free(ans->value);
		free(ans);
		ans = next;
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;
	unsigned char c;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}

void canonicalize_name(char *name) {
	/*
	 * Canonicalize name in place.  Change all upper-case characters to
	 * lower case and remove the trailing dot if there is any.  If the name
	 * passed is a single dot, "." (representing the root zone), then it
	 * should stay the same.
	 *
	 * INPUT:  name: the domain name that should be canonicalized in place
	 */
	
	int namelen, i;

	// leave the root zone alone
	if (strcmp(name, ".") == 0) {
		return;
	}

	namelen = strlen(name);
	// remove the trailing dot, if any
	if (name[namelen - 1] == '.') {
		name[namelen - 1] = '\0';
	}

	// make all upper-case letters lower case
	for (i = 0; i < namelen; i++) {
		if (name[i] >= 'A' && name[i] <= 'Z') {
			name[i] += 32;
		}
	}
}

void print_req_array(char* array){
	int i = 0;
	for (i=0;i < strlen(array);i++) {
    	printf("%x\n",array[i]);
	}
}

int name_ascii_to_wire(char *name, unsigned char *wire) {
	/* 
	 * Convert a DNS name from string representation (dot-separated labels)
	 * to DNS wire format, using the provided byte array (wire).  Return
	 * the number of bytes used by the name in wire format.
	 *
	 * INPUT:  name: the string containing the domain name
	 * INPUT:  wire: a pointer to the array of bytes where the
	 *              wire-formatted name should be constructed
	 * OUTPUT: the length of the wire-formatted name.
	 */
	// printf("Converting DNS Name into ascii\n");
	canonicalize_name(name);

    char* token; 
    char* rest = name; 
	int index = 0;
  
    while ((token = strtok_r(rest, ".", &rest))) {
		// printf("length = %ld ",strlen(token));
        // printf("%s\n", token); 
		wire[index] = (char) (strlen(token));
		index++;

		for(int i = 0; i < strlen(token); i++){
			wire[index] = (char) token[i] & 0xff;
			// printf("wire[%d] = %x\n", index, wire[index]);
			index++;
		}
	}
	wire[index] = '\0';
	index++;


	return index;
}

char *name_ascii_from_wire(unsigned char *wire, int *indexp) {
	/* 
	 * Extract the wire-formatted DNS name at the offset specified by
	 * *indexp in the array of bytes provided (wire) and return its string
	 * representation (dot-separated labels) in a char array allocated for
	 * that purpose.  Update the value pointed to by indexp to the next
	 * value beyond the name.
	 *
	 * INPUT:  wire: a pointer to an array of bytes
	 * INPUT:  indexp, a pointer to the index in the wire where the
	 *              wire-formatted name begins
	 * OUTPUT: a string containing the string representation of the name,
	 *              allocated on the heap.
	 */
	// printf("Getting answer name from wire..\n");
	// print_bytes(wire, 190);
	unsigned char c = wire[*indexp];
	char* name = malloc(500);
	int name_index = 0;

	// printf("initial index = %d\n", *indexp);

	while(c != 0){

		if (c < 192){
			// *indexp = (int) c;
			int additional_index = 0;
			c = wire[*indexp];
			*indexp += 1;
			// printf("c = %x\n", c);
			for (int i = 0; i < c; i++){
			//  printf("%x\n", *indexp + i);
				name[name_index + i + 1] = 0;
				name[name_index + i] = wire[*indexp + i];
				additional_index++;
							// printf("name = %s\n", name);

			}
			name_index += additional_index;
			// printf("additional_index = %d", additional_index);
			*indexp += additional_index;
			// printf("indexp = %x\n", *indexp);
			// printf("wire = %x\n",wire[*indexp]);
			if(wire[*indexp]){
				name[name_index] = '.';
			}
			name_index++;
			c = wire[*indexp];
			// printf("name = %s\n", name);

		}
		else {
			// indexp++;
			// printf("is pointer\n");
			*indexp += 1;
			unsigned char pointer_index = wire[*indexp];
			// printf("c = %x\n", c);


			c = pointer_index;

			*indexp = (int) c;
						// printf("name = %s\n", name);

		}
	}
	// printf("name_to_ascii = %s\n", name);
	return name;
}

dns_rr rr_from_wire(unsigned char *wire, int *indexp, int query_only) {
	/* 
	 * Extract the wire-formatted resource record at the offset specified by
	 * *indexp in the array of bytes provided (wire) and return a 
	 * dns_rr (struct) populated with its contents. Update the value
	 * pointed to by indexp to the next value beyond the resource record.
	 *
	 * INPUT:  wire: a pointer to an array of bytes
	 * INPUT:  indexp: a pointer to the index in the wire where the
	 *              wire-formatted resource record begins
	 * INPUT:  query_only: a boolean value (1 or 0) which indicates whether
	 *              we are extracting a full resource record or only a
	 *              query (i.e., in the question section of the DNS
	 *              message).  In the case of the latter, the ttl,
	 *              rdata_len, and rdata are skipped.
	 * OUTPUT: the resource record (struct)
	 */
	int beginning_index = *indexp;
	// print_bytes(wire, 89);

	char* answer_name = name_ascii_from_wire(wire, indexp);


	dns_rr_type type = wire[beginning_index + 3];
	dns_rr_class class = wire[beginning_index + 5];
	unsigned char* data = malloc(sizeof(unsigned char*) * 100);
	dns_rdata_len length = wire[beginning_index + 11];
	*indexp = beginning_index + 12 + length;

	for(int i = 0; i < length; i++){
		*(data + i) = (wire[beginning_index + 12 + i] );
	}
		//  printf("beginning length = %d\n", length);

	// printf("Before big if \n");
	if (type == 5){
		// printf("beginning index = %d\n", beginning_index + 11);
		int true_length, pointer_found = 0;
		unsigned char pointer_index;
		int index = 0;

		for(int i = 0; i < length; i++){
			// printf("i = %d\n",i);
			unsigned char x = wire[beginning_index + 12 + i];
			true_length += 1;
			// printf("wire = %x\n", wire[beginning_index + 12 + i]);
			// printf("%x\n", x);
			if(x == 0xC0 && !pointer_found){
				// printf("HERE______________\n");
				// printf("i = %d\n", i);
				pointer_index = wire[beginning_index + 12 + i + 1];
							//  printf/("i = %d\n", i);
				index = i;
				pointer_found = 1;
			}
		}
		// printf("Out of loop");
		if (pointer_found){
			true_length = index;
		}

		unsigned char c = wire[(int)pointer_index];
			//  printf("pointer_index = %x\n", pointer_index);
			 int temp = 0;

		while(c != 0 && pointer_index){
			// printf("c = %x\n", c);
						//  printf("pointer_index = %d\n", pointer_index);


			if (c < 192){
				temp = (int) pointer_index;
				int jump = c + 1;
				temp += jump;
				pointer_index = (unsigned char) temp;

				true_length += (int) c;
				c = wire[temp];

			}
			else {
				printf("ERROR: SOMETHING WENT WRONG\n");
			}
		}
		length = true_length;
	}

	dns_rr resource = {answer_name, type, class, 0, length, data};

	return resource;

}


int rr_to_wire(dns_rr rr, unsigned char *wire, int query_only) {
	/* 
	 * Convert a DNS resource record struct to DNS wire format, using the
	 * provided byte array (wire).  Return the number of bytes used by the
	 * name in wire format.
	 *
	 * INPUT:  rr: the dns_rr struct containing the rr record
	 * INPUT:  wire: a pointer to the array of bytes where the
	 *             wire-formatted resource record should be constructed
	 * INPUT:  query_only: a boolean value (1 or 0) which indicates whether
	 *              we are constructing a full resource record or only a
	 *              query (i.e., in the question section of the DNS
	 *              message).  In the case of the latter, the ttl,
	 *              rdata_len, and rdata are skipped.
	 * OUTPUT: the length of the wire-formatted resource record.
	 *
	 */
}

unsigned short create_dns_query(char *qname, dns_rr_type qtype, unsigned char *wire) {
	/* 
	 * Create a wire-formatted DNS (query) message using the provided byte
	 * array (wire).  Create the header and question sections, including
	 * the qname and qtype.
	 *
	 * INPUT:  qname: the string containing the name to be queried
	 * INPUT:  qtype: the integer representation of type of the query (type A == 1)
	 * INPUT:  wire: the pointer to the array of bytes where the DNS wire
	 *               message should be constructed
	 * OUTPUT: the length of the DNS wire message
	 */

	unsigned char header[12] = {0};
	unsigned char query[100] = {0};

	unsigned short query_id = random();
	header[0] = (char)(query_id & 0xff);
	header[1] = (char)((query_id & 0xff00) >> 2);
	header[2] = 0x01;
	header[3] = 0x00;
	header[4] = 0x00;
	header[5] = 0x01;
	header[6] = 0x00;
	header[7] = 0x00;
	header[8] = 0x00;
	header[9] = 0x00;
	header[10] = 0x00;
	header[11] = 0x00;
	
	int query_length = name_ascii_to_wire(qname, query);

	query[query_length] = 0x00;
	query_length++;
	query[query_length] = 0x01;
	query_length++;
	query[query_length] = 0x00;
	query_length++;
	query[query_length] = 0x01;
	query_length++;

	int i = 0;
	for(i; i < 12; i++){
		wire[i] = header[i];
	}

	for(i = 0; i < query_length; i++){
		wire[12 + i] = query[i];
		// printf("wire = %x, query = %x\n", wire[12+i], query[i]);
	}
	// print_bytes(wire, query_length + 12);


	return query_length + 12;
}

void print_list(dns_answer_entry* head){
	dns_answer_entry* current = head;

	while (current != NULL){
		printf("%s\n", current->value);
		current = current->next;
	}
}

dns_answer_entry *get_answer_address(char *qname, dns_rr_type qtype, unsigned char *wire) {
	/* 
	 * Extract the IPv4 address from the answer section, following any
	 * aliases that might be found, and return the string representation of
	 * the IP address.  If no address is found, then return NULL.
	 *
	 * INPUT:  qname: the string containing the name that was queried
	 * INPUT:  qtype: the integer representation of type of the query (type A == 1)
	 * INPUT:  wire: the pointer to the array of bytes representing the DNS wire message
	 * OUTPUT: a linked list of dns_answer_entrys the value member of each
	 * reflecting either the name or IP address.  If
	 */
	int number_of_answers = (int)wire[7];
	// printf("number_of_answers = %d\n", number_of_answers);
	// print_bytes(wire, 114);
	int cname_five = 0;
	int prev_record_length = 0;

	if(number_of_answers == 0){
		return NULL;
	}

	dns_answer_entry* head = malloc(sizeof(dns_answer_entry));
	head->next = NULL;
	head->value = "0";
	int index = 12 + strlen(qname) + 6;
	

	for(int i = 0; i < number_of_answers; i++){
		dns_rr record = rr_from_wire(wire, &index, 0);

			if(cname_five){
				// printf("qname = %s\n", qname);
				dns_answer_entry* cname_entry = malloc(sizeof(dns_answer_entry));
				char* answer = malloc(100);
				memcpy(answer, qname, prev_record_length + 1);
				// printf("answer = %s\n", record.name);
				cname_entry->value = answer;
				cname_entry->next = NULL;
				//  printf("record.length = %d\n", prev_record_length + 1);
				dns_answer_entry* parent = head;

				while(parent->next != NULL){
					parent = parent->next;
				}

				parent->next = cname_entry;

				cname_five = 0;
			}

		if((strcmp(record.name, qname) == 0) && record.type == 1){
			// printf("record.type = 1\n");
			// printf("cname_five = %d\n", cname_five);

			int ip_numbers[100] = {0};
			for(int j = 0; j < record.rdata_len; j++){
				int ip = (int) record.rdata[j];
				ip_numbers[j] = ip;
			}
			char* answer = malloc(100);
			sprintf(answer, "%i.%i.%i.%i", ip_numbers[0], ip_numbers[1], ip_numbers[2], ip_numbers[3]);
			dns_answer_entry* new_entry = malloc(sizeof(dns_answer_entry));
			new_entry->value = answer;
			new_entry->next = NULL;
			// printf("%p\n", new_entry);
			dns_answer_entry* parent_entry = head;

			while(parent_entry->next != NULL){
				parent_entry = parent_entry->next;
			}

			parent_entry->next = new_entry;
			// print_list(head);
			// printf("%p\n", head->next);

		}
		else if ((strcmp(record.name, qname) == 0) && record.type == 5){

			// printf("qname = %p\n", qname);
			// printf("record.name = %p\n", record.name);
			qname = record.name;
			memcpy(qname, record.name, record.rdata_len);
			cname_five = 1;
			prev_record_length = record.rdata_len;
			
		}

		free(record.rdata);
		free(record.name);

	}
	dns_answer_entry* ptr_to_return = head->next;
	free(head);
	return ptr_to_return;
}



int send_recv_message(unsigned char *request, int requestlen, unsigned char *response, char *server, unsigned short port) {
	/* 
	 * Send a message (request) over UDP to a server (server) and port
	 * (port) and wait for a response, which is placed in another byte
	 * array (response).  Create a socket, "connect()" it to the
	 * appropriate destination, and then use send() and recv();
	 *
	 * INPUT:  request: a pointer to an array of bytes that should be sent
	 * INPUT:  requestlen: the length of request, in bytes.
	 * INPUT:  response: a pointer to an array of bytes in which the
	 *             response should be received
	 * OUTPUT: the size (bytes) of the response received
	 */
	// printf("Sending the query to the DNS resolver\n");
	struct sockaddr_in address;
	socklen_t peer_addr_len;
	int query_length, sfd, nread;
	unsigned char buffer[500];

	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
  	address.sin_port = htons(port);
  	address.sin_addr.s_addr = inet_addr(server);
	peer_addr_len = sizeof(struct sockaddr_in);


	if ((sfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
		printf("ERROR: CREATING SOCKET!\n");
		printf("ERROR: %s\n", strerror(errno));
		return -1;
	}
		// printf("Created Socket! \n");
		// printf("Attempting to connect... \n");


	if (connect(sfd, (struct sockaddr*) &address, peer_addr_len) < 0){
		printf("ERROR: CONNECTING SOCKET\n");
		printf("ERROR: %s\n", strerror(errno));
		return -1; 
	}
		// printf("Connected! \n");


	if ((send(sfd, request, requestlen, 0) < 0 )){
		printf("ERROR: SENDING REQUEST\n");
	}

	nread = recv(sfd, response, 500, 0);

	if (nread == -1){
		printf("ERROR: RETREIVING ANSWER\n");
		return -1;
	}
    if (nread == 0){
		printf("ERROR: NOTHING RECIEVED\n");
		return 0;
	}

	response[nread] = '\0';


	return nread;

}

void print_query(unsigned char* query, int query_length)
{
		for(int i = 0; i < query_length; i++){
		printf("request[%d] = %x\n", i, query[i]);
	}
}

void print_response(unsigned char* response, int response_size)
{
	for(int i = 0; i < response_size; i++){
		printf("response[%d] = %x\n", i, response[i]);
	}
}

dns_answer_entry *resolve(char *qname, char *server, char *port) 
{
	unsigned char wire[100] = {0};
	char query_name[100] = {0};
	unsigned char* response = (unsigned char*) malloc(1024);
	unsigned short converted_port = 0;

	strcpy(query_name, qname);
	int query_length = create_dns_query(query_name, 0x01, wire);
	converted_port = atoi(port);
	int response_length = send_recv_message(wire, query_length, response, server, converted_port);

	dns_answer_entry* answer = get_answer_address(qname, 0x01, response);
	free(response);
	return answer;
}

int main(int argc, char *argv[]) {
	char *port;
	dns_answer_entry *ans_list, *ans;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <domain name> <server> [ <port> ]\n", argv[0]);
		exit(1);
	}
	if (argc > 3) {
		port = argv[3];
	} else {
		port = "53";
	}
	ans = ans_list = resolve(argv[1], argv[2], port);
	while (ans != NULL) {
		printf("%s\n", ans->value);
		ans = ans->next;
	}
	if (ans_list != NULL) {
		free_answer_entries(ans_list);
	}
}

/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"
#include <string.h>
#include "contiki.h"
#include "rest-engine.h"
#include "dev/leds.h"
#include "er-coap.h"

char buffer_declarado[3][REST_MAX_CHUNK_SIZE];
uint16_t tamanho_do_buffer[3];

static void res_post_put_handler(void *request, void *response, uint8_t *buffer,
                                 uint16_t preferred_size, int32_t *offset);

static void res_get_handler(void *request, void *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);

/* A simple echo with PUT/POST/GET */
RESOURCE(res_echo, "title=\"Echo\";rt=\"Control\"", res_get_handler,
         res_post_put_handler, res_post_put_handler, NULL);

static void res_post_put_handler(void *request, void *response, uint8_t *buffer,
                                 uint16_t preferred_size, int32_t *offset)
{
    printf("POST/PUT called.\n");
    //converte o payload recebido por PUT em um pacote CoAP
    coap_packet_t * const coap_req = (coap_packet_t *) request;
    uint8_t buffer_ptr = 0;
    //verifica se o payload enviado nao eh muito grande para a requisicao
    if (coap_req->payload_len > REST_MAX_CHUNK_SIZE)
    {
        printf("*** ERROR *** Payload is to big!\n");
        //caso for muito grande, simplesmente configura a resposta como BAD_REQUEST e retorna
        REST.set_response_status(response, REST.status.BAD_REQUEST);
        return;
    }
    else
    {
        const char *bnumber = NULL;
        const char *message = NULL;
        int32_t buffernumber = 0;
        /* The query string can be retrieved by rest_get_query() or parsed for its key-value pairs. */
        if (REST.get_post_variable(request, "buffernumber", &bnumber))
        {
            buffernumber = atoi(bnumber);
        }
        else
        {
            printf("*** ERROR *** 'buffernumber' parameter not found!\n");
            REST.set_response_status(response, REST.status.BAD_OPTION);
            return;
        }
        if (!REST.get_post_variable(request, "message", &message))
        {
            printf("*** ERROR *** 'message' parameter not found!\n");
            REST.set_response_status(response, REST.status.BAD_OPTION);
            return;
        }
        if ((buffernumber < 1) || (buffernumber > 3))
        {
            printf("*** ERROR *** invalid value for 'buffernumber' parameter!\n");
            //caso for muito grande, simplesmente configura a resposta como BAD_REQUEST e retorna
            REST.set_response_status(response, REST.status.BAD_OPTION);
            return;
        }
        else
        {
            buffernumber = buffernumber - 1;
            printf("buffernumber = %i\n", buffernumber);
            printf("message = %s\n", message);
            printf("payload = %s\n", coap_req->payload);
            //caso contrario, copia a mensagem enviada para o buffer criado
            //memcpy((void*)buffer_declarado[buffernumber], (void*)coap_req->payload, coap_req->payload_len);
            memcpy((void*) buffer_declarado[buffernumber], (void*) message,
                   strlen(message));
            //salva tambem o tamanho da mensagem recebida (para uso futuro)
            tamanho_do_buffer[buffernumber] = coap_req->payload_len;
        }
    }
}

static void res_get_handler(void *request, void *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset)
{
    printf("GET called.\n");
    uint32_t i;
    uint8_t etag = 0;
    //configura o tipo de conteudo da mensagem
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    //etag eh uma propriedade que eh utilizada pelos servidores de cache para saber se uma mensagem mudou
    //duas mensagens com o mesmo valor devem ter o mesmo etag
    const char *bnumber = NULL;
    int32_t buffernumber = 0;
    /* The query string can be retrieved by rest_get_query() or parsed for its key-value pairs. */
    if (REST.get_query_variable(request, "buffernumber", &bnumber))
    {
        buffernumber = atoi(bnumber);
    }
    else
    {
        printf("*** ERROR *** 'buffernumber' parameter not found!\n");
        REST.set_response_status(response, REST.status.BAD_OPTION);
        return;
    }
    if ((buffernumber < 1) || (buffernumber > 3))
    {
        //caso for muito grande, simplesmente configura a resposta como BAD_REQUEST e retorna
        printf("*** ERROR *** invalid value for 'buffernumber' paramater!\n");
        REST.set_response_status(response, REST.status.BAD_OPTION);
        return;
    }
    else
    {
        buffernumber = buffernumber - 1;
        printf("buffernumber = %i\n", buffernumber);
        for (i = 0; i < tamanho_do_buffer[buffernumber]; i++)
        {
            //neste caso utilizamos um checksum simples como etag, mas o usuario pode usar o que quiser
            etag += buffer_declarado[buffernumber][i];
        }
        REST.set_header_etag(response, (uint8_t *) &etag, 1);
        //configura o payload a ser retornado
        REST.set_response_payload(response, buffer_declarado[buffernumber],
                                  tamanho_do_buffer[buffernumber]);
    }
}

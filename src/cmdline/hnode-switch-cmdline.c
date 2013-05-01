#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <glib.h>

#include <hnode-1.0/hnode-browser.h>
#include <hnode-1.0/hnode-pktsrc.h>
#include <hnode-1.0/hnode-cfginf.h>
#include <hnode-1.0/hnode-uid.h>

#include "hnode-switch-interface.h"
#include "hnode-switch-cmdline.h"

static gboolean  list       = FALSE;
static gboolean  switch_on  = FALSE;
static gboolean  switch_off = FALSE;
static gchar    *target       = NULL;
static gchar    *command      = NULL;

static GOptionEntry entries[] = 
{
  { "list", 'l', 0, G_OPTION_ARG_NONE, &list, "Discover controllable switches.  Output a list upon exit.", NULL },
  { "target", 't', 0, G_OPTION_ARG_STRING, &target, "The switch to target with additional operations.", "SwitchID" },
  { "on", 'o', 0, G_OPTION_ARG_NONE, &switch_on, "Request the switch to turn on.", NULL },
  { "off", 'x', 0, G_OPTION_ARG_NONE, &switch_off, "Request the switch to turn off", NULL }, 
  { "command", 'c', 0, G_OPTION_ARG_STRING, &command, "Request a generic switch action", "command string" },
  { NULL }
};
 
enum
{
    OPER_NONE,
    OPER_LIST,
    OPER_ON,
    OPER_OFF,
    OPER_GENERIC
};

enum
{
    SWNODE_STATE_NOINIT,
    SWNODE_STATE_SWLIST,
    SWNODE_STATE_READY,
};

typedef struct HNodeSwitchNodeRecord
{
    GHNodeBrowserHNode *BrowserNode;
    guint32             EPIndex;
    guint32             State;
    GHNodeAddress      *EPAddrObj;
    guint32             NodeID;
}HNODE_SWITCH_NODE;

typedef struct HNodeSwitchRecord
{
    guint32            SwitchIDLength;
    gchar             *SwitchIDStr;
    guint32            NameLength;
    gchar             *NameStr;
    guint32            SwitchIndex;
    guint32            CapFlags;
    HNODE_SWITCH_NODE *ParentNode;
}HNODE_SWITCH;

typedef struct HNodeSwitchRequest
{
    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
}HNODE_SWITCH_REQUEST;

typedef struct HNodeSwitchCmdlineContext
{
   GMainLoop     *loop; 
   GHNodeBrowser *Browser;

   //GHNodeUID          *TargetUID;
   //GHNodeBrowserHNode *CurHNode;

   GHNodePktSrc       *SwitchSource;
   GHNodeAddress      *SwitchAddrObj;

   guint32 Operation;

   guint32 ReqTag;
   guint32 ReqState;
   guint32 ReqTID;

   GArray *SwitchNodeArray;
   GArray *SwitchArray;

   GList  *ReqList;
}CONTEXT;

// Create a global program context.
CONTEXT  gContext;

/* Node Config Request Timeout Event */
static gboolean
swcfg_request_timeout (void *userdata)
{
    // Signal a communication error with the node
    g_error("Node Request Timeout...\n");    

    return FALSE;
}

gboolean
swcfg_request_switch_list( CONTEXT *Context, HNODE_SWITCH_NODE *SwitchNode )
{
    GHNodePacket *Packet;

    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
    
    HNODE_SWITCH_REQUEST *ReqRecord;

    // Fire off the status request to the configuration interface.
    g_message ("swcfg_request_switch_list\n");

    Packet    = g_hnode_packet_new();
    ReqRecord = g_malloc(sizeof(HNODE_SWITCH_REQUEST));

    // Reset the insertion point to zero.
    g_hnode_packet_reset(Packet);
        
    // Set-up the destination address.
    g_hnode_packet_assign_addrobj(Packet, SwitchNode->EPAddrObj);

    // Set-up the request record
    Context->ReqTag += 1;
    ReqRecord->ExpectedResponse = SWPKT_SWITCH_LIST_REPLY;
    ReqRecord->ReqTag      = Context->ReqTag;
    ReqRecord->Switch      = NULL;
    ReqRecord->RequestNode = SwitchNode;

    Context->ReqList = g_list_append(Context->ReqList, ReqRecord);

    // Build the packet IU.
    g_hnode_packet_set_uint(Packet, SWPKT_SWITCH_LIST_REQUEST);
    g_hnode_packet_set_uint(Packet, ReqRecord->ReqTag);

    // Send the packet
    g_hnode_pktsrc_send_packet(Context->SwitchSource, Packet);

    // Succeeded
    return FALSE;
}

gboolean
swcfg_request_switch_on_off( CONTEXT *Context, HNODE_SWITCH *SwitchRecord, gboolean OnRequest )
{
    GHNodePacket *Packet;

    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
    
    HNODE_SWITCH_REQUEST *ReqRecord;

    // Fire off the status request to the configuration interface.
    g_message ("swcfg_request_switch_on\n");

    Packet    = g_hnode_packet_new();
    ReqRecord = g_malloc(sizeof(HNODE_SWITCH_REQUEST));

    // Reset the insertion point to zero.
    g_hnode_packet_reset(Packet);
        
    // Set-up the destination address.
    g_hnode_packet_assign_addrobj(Packet, SwitchRecord->ParentNode->EPAddrObj);

    // Set-up the request record
    Context->ReqTag += 1;
    ReqRecord->ExpectedResponse = SWPKT_SWITCH_CMD_REPLY;
    ReqRecord->ReqTag      = Context->ReqTag;
    ReqRecord->Switch      = SwitchRecord;
    ReqRecord->RequestNode = SwitchRecord->ParentNode;

    Context->ReqList = g_list_append(Context->ReqList, ReqRecord);

    // Build the packet IU.
    g_hnode_packet_set_uint(Packet, SWPKT_SWITCH_CMD_REQUEST);
    g_hnode_packet_set_uint(Packet, ReqRecord->ReqTag);

    g_hnode_packet_set_short(Packet, SwitchRecord->SwitchIndex);  // SwitchIndex
    g_hnode_packet_set_short(Packet, OnRequest ? SWINF_CMD_TURN_ON : SWINF_CMD_TURN_OFF);  // Requested Action
    g_hnode_packet_set_short(Packet, 0);  // Action Parameter 1
    g_hnode_packet_set_short(Packet, 0);  // Action Paremeter 2

    // Send the packet
    g_hnode_pktsrc_send_packet(Context->SwitchSource, Packet);

    // Succeeded
    return FALSE;
}

void 
swcfg_hnode_rx(GHNodePktSrc *Browser, GHNodePacket *Packet, gpointer data)
{
    CONTEXT *Context = data;
    guint    DataLength;
    guint    PktType;
    guint    ReqTag;
    guint    i, j;
    guint    SwitchCount;
    gchar    AttrStr[256];
    guint    MemSegID;
    guint8   TmpBuf[512];

    GList    *CurReq;
    HNODE_SWITCH_REQUEST *ReqRecord;
    HNODE_SWITCH SwitchRecord;

    g_message ("swcfg_hnode_rx: called!\n");

    // Turn off the request timer if one was enabled.
    if( Context->ReqTID )
    {
        g_source_remove( Context->ReqTID ); 
        Context->ReqTID = 0;
    }

    // Parse the rx frame
    DataLength = g_hnode_packet_get_data_length(Packet);

    // Pull out the common fields.
    PktType    = g_hnode_packet_get_uint(Packet);
    ReqTag     = g_hnode_packet_get_uint(Packet);

    g_message ("swcfg_hnode_rx -- PktLength:%d  PktType:0x%x  ReqTag:0x%x\n", DataLength, PktType, ReqTag);

    // Find the request record that this reply is for.
    ReqRecord = NULL;
    CurReq = g_list_first(Context->ReqList);
    while( CurReq )
    {
        if(((HNODE_SWITCH_REQUEST *)CurReq->data)->ReqTag == ReqTag)
        {
           ReqRecord = CurReq->data;
           break;         
        }    
        CurReq = g_list_next(CurReq);
    }

    // Error check
    if( (ReqRecord == NULL) || (PktType != ReqRecord->ExpectedResponse) )
    {
        // Free the RX frame
        g_object_unref(Packet);
        return;
    }

    // Act on the new packet
    switch( PktType )
    {
        case SWPKT_SWITCH_LIST_REPLY:
            // Number of switch records from this node.
            SwitchCount = g_hnode_packet_get_short(Packet); // Number of switch records.

            for(i = 0; i < SwitchCount; i++)
            {
                SwitchRecord.SwitchIndex   = g_hnode_packet_get_short(Packet);  // Local SwitchID

  		        SwitchRecord.SwitchIDLength = g_hnode_packet_get_short(Packet); // Switch Name Length
                SwitchRecord.SwitchIDStr = NULL;
                if( SwitchRecord.SwitchIDLength )
                {
                    g_hnode_packet_get_bytes(Packet, TmpBuf, SwitchRecord.SwitchIDLength);
                    TmpBuf[SwitchRecord.SwitchIDLength] = '\0';
                    SwitchRecord.SwitchIDStr = g_strdup(TmpBuf);
                }

  		        SwitchRecord.NameLength = g_hnode_packet_get_short(Packet); // Switch Name Length
                SwitchRecord.NameStr = NULL;
                if( SwitchRecord.NameLength )
                {
                    g_hnode_packet_get_bytes(Packet, TmpBuf, SwitchRecord.NameLength);
                    TmpBuf[SwitchRecord.NameLength] = '\0';
                    SwitchRecord.NameStr = g_strdup(TmpBuf);
                }

                SwitchRecord.CapFlags   = g_hnode_packet_get_uint(Packet); 

                SwitchRecord.ParentNode = ReqRecord->RequestNode;

                g_print("Adding Switch: %s\n", SwitchRecord.NameStr);

                Context->SwitchArray = g_array_append_val(Context->SwitchArray, SwitchRecord);
            }
        break;

        case SWPKT_SWITCH_CMD_REPLY:
            g_print("Cmd Result: 0x%x\n", g_hnode_packet_get_short(Packet));
            
            g_main_loop_quit( Context->loop );
        break;
    }

    // Free the ReqRecord
    Context->ReqList = g_list_remove(Context->ReqList, CurReq);
    g_free(ReqRecord);

    // Free the RX frame
    g_object_unref(Packet);

	return;
}
 
void 
swcfg_hnode_tx(GHNodePktSrc *Browser, GHNodePacket *Packet, gpointer data)
{
    g_message ("swcfg_hnode_tx: called!\n");

    // Free the TX frame
    g_object_unref(Packet);

	return;
}

void 
swcfg_output_switch_list( CONTEXT *Context )
{
    HNODE_SWITCH *SwitchRecord;
    guint         i;
    gchar         NodeStr[128];
    GHNodeUID    *NodeUID;

    g_print("\n\nSwitch List:\n");     

    // Cycle through the nodes starting at the first.
    for( i = 0; i < Context->SwitchArray->len; i++)
    {
        SwitchRecord = &g_array_index (Context->SwitchArray, HNODE_SWITCH, i);

        //NodeUID = g_hnode_browser_hnode_get_uid_objptr(SwitchRecord->ParentNode->BrowserNode);
        //g_hnode_uid_get_uid_as_string(NodeUID, NodeStr);

        g_print("\tSwitch(%d): %s  ID:%s\n", SwitchRecord->SwitchIndex, SwitchRecord->NameStr, SwitchRecord->SwitchIDStr);
    }

}

search_timeout( gpointer userdata )
{
    CONTEXT *Context = userdata;
    guint i;
    HNODE_SWITCH *SwitchRecord;

    // Determine which operation to perform.
    if( list )
    {
        // Request the list of switches
        //gboolean swcfg_request_switch_list( CONTEXT *Context, HNODE_SWITCH_NODE *SwitchNode )

        // Dump a list of discovered nodes.
        swcfg_output_switch_list(Context);

        goto done;
    }
    else if( switch_on || switch_off || command  )
    {

        // Find the targeted node.
        for( i = 0; i < Context->SwitchArray->len; i++)
        {
            SwitchRecord = &g_array_index (Context->SwitchArray, HNODE_SWITCH, i);

            if( g_strcmp0(SwitchRecord->SwitchIDStr, target) == 0 )
            {
                g_print("Target Switch(%d): %s  ID:%s\n", SwitchRecord->SwitchIndex, SwitchRecord->NameStr, SwitchRecord->SwitchIDStr);

                if( switch_on )
                {
                    swcfg_request_switch_on_off( Context, SwitchRecord, TRUE );
                }
                else if( switch_off )
                {
                    swcfg_request_switch_on_off( Context, SwitchRecord, FALSE );
                }
                else
                {
                    goto done;
                }
            }

        }

    }
    else
    {
        // Nothing to do so bail
        goto done;
    }
 
    return;

done:

    // Quit the application 
    g_main_loop_quit( Context->loop );

}

static void
enumeration_complete( GHNodeBrowser *Browser, GHNodeBrowserMPoint *MPoint, void *userdata )
{
    CONTEXT *Context = userdata;

 
    g_print("Enumeration Complete: %d\n", Context->SwitchArray->len);

    // Set a timer to give time to get responses from the switch nodes.
    g_timeout_add( 100, search_timeout, Context );
}


static void
mgmt_add (GHNodeBrowser *Browser, GHNodeBrowserMPoint *MPoint, void *userdata)
{
    CONTEXT *Context = userdata;
}

static void
hnode_add (GHNodeBrowser *Browser, GHNodeBrowserHNode *hnode, void *userdata)
{
    CONTEXT *Context = userdata;
    guint  i;
    guint32  EPIndex;
    HNODE_SWITCH_NODE  SwitchNode;
    HNODE_SWITCH_NODE *SwitchPtr;

    g_print("hnode_add\n");

    // Check if this hnode is a switch interface node.
    if( g_hnode_browser_hnode_supports_interface(hnode, "hnode-switch-interface", &EPIndex) == TRUE )
    { 
        // If it is then remember it's HNode and request it's switch list.
        g_print("Found switch Hnode -- EPIndex: %d\n", EPIndex);    

        SwitchNode.BrowserNode = hnode;
        SwitchNode.EPIndex     = EPIndex;
        SwitchNode.State       = SWNODE_STATE_SWLIST;

        // Get the address object to use when building outbound packets.
        g_hnode_browser_hnode_endpoint_get_address(hnode, EPIndex, &SwitchNode.EPAddrObj); 

        // Add the new switch node to the array
        Context->SwitchNodeArray = g_array_append_val(Context->SwitchNodeArray, SwitchNode);

        // Request a switch list from the node.
        SwitchPtr = &g_array_index(Context->SwitchNodeArray, HNODE_SWITCH_NODE, Context->SwitchNodeArray->len-1);
        swcfg_request_switch_list( Context, SwitchPtr );

    }
}


int
main (int argc, char *argv[])
{
    GMainLoop *loop = NULL;
    GHNodeBrowser *Browser;
    GError *error = NULL;
    GOptionContext *cmdline_context;

    // Initialize some context.
    gContext.SwitchSource  = NULL;
    gContext.SwitchAddrObj = NULL;
    gContext.ReqTag        = 0;
    gContext.ReqState      = 0;
    gContext.ReqTID        = 0;

    gContext.SwitchNodeArray = g_array_new( FALSE, TRUE, sizeof(HNODE_SWITCH_NODE) );
    gContext.SwitchArray = g_array_new( FALSE, TRUE, sizeof(HNODE_SWITCH) );

    gContext.ReqList = NULL;

    // Initialize the gobject type system
    g_type_init();

    gContext.loop    = g_main_loop_new (NULL, FALSE);

    g_print("Context: 0x%x\n", (guint32)&gContext);

    // Parse any command line options.
    cmdline_context = g_option_context_new ("- command line interface to hnode switch controllers.");
    g_option_context_add_main_entries (cmdline_context, entries, NULL); // GETTEXT_PACKAGE);
    g_option_context_parse (cmdline_context, &argc, &argv, &error);

    // Allocate a socket that will be used to converse with a the HNode configuration interface.
    gContext.SwitchSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    g_signal_connect(gContext.SwitchSource, "rx_packet", G_CALLBACK(swcfg_hnode_rx), &gContext);
    g_signal_connect(gContext.SwitchSource, "tx_packet", G_CALLBACK(swcfg_hnode_tx), &gContext);

    g_hnode_pktsrc_start( gContext.SwitchSource );

    // Setup the hnode browser
	gContext.Browser = g_hnode_browser_new ();
	
    g_signal_connect(gContext.Browser, "hnode-add", G_CALLBACK(hnode_add), &gContext);
    g_signal_connect(gContext.Browser, "mgmt-add", G_CALLBACK(mgmt_add), &gContext);
    g_signal_connect(gContext.Browser, "enumeration-complete", G_CALLBACK(enumeration_complete), &gContext);
            
    // Start up the discovery stuff.
    g_hnode_browser_start(gContext.Browser);
    
    // Error check the command line options
    if( list )
    {
        gContext.Operation = OPER_LIST;
    }
    else if( switch_on || switch_off )
    {
        // Ensure that a target switch has been specified.
        if( target == NULL )
        {
            g_error("On/Off/Command operations require that a target switch is specified.\n");
            goto fail;
        }

        // Allocate and setup the target UID
        g_printf("target: %s\n", target);
        gContext.Operation = switch_on ? OPER_ON : OPER_OFF;
        //gContext.TargetID = strtol(target, NULL, 0);
    }
    else if( command )
    {
        gContext.Operation = OPER_GENERIC;
        g_print("command option not yet implemented.");
    }
    else
    {
        g_print("No action requested,  exiting....\n");
        // No option selected -- Just exit.
        goto fail;
    }
   
    /* Start the GLIB Main Loop */
    g_main_loop_run (gContext.loop);

    fail:
    /* Clean up */
    g_main_loop_unref (gContext.loop);
    g_object_unref(gContext.Browser);

    return 0;
}


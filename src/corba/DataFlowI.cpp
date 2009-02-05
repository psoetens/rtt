/***************************************************************************
  tag: FMTC  Tue Mar 11 21:49:24 CET 2008  DataFlowI.cpp

                        DataFlowI.cpp -  description
                           -------------------
    begin                : Tue March 11 2008
    copyright            : (C) 2008 FMTC
    email                : peter.soetens@fmtc.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public                   *
 *   License as published by the Free Software Foundation;                 *
 *   version 2 of the License.                                             *
 *                                                                         *
 *   As a special exception, you may use this file as part of a free       *
 *   software library without restriction.  Specifically, if other files   *
 *   instantiate templates or use macros or inline functions from this     *
 *   file, or you compile this file and link it with other files to        *
 *   produce an executable, this file does not by itself cause the         *
 *   resulting executable to be covered by the GNU General Public          *
 *   License.  This exception does not however invalidate any other        *
 *   reasons why the executable file might be covered by the GNU General   *
 *   Public License.                                                       *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


// -*- C++ -*-
//
// $Id$

// ****  Code generated by the The ACE ORB (TAO) IDL Compiler ****
// TAO and the TAO IDL Compiler have been developed by:
//       Center for Distributed Object Computing
//       Washington University
//       St. Louis, MO
//       USA
//       http://www.cs.wustl.edu/~schmidt/doc-center.html
// and
//       Distributed Object Computing Laboratory
//       University of California at Irvine
//       Irvine, CA
//       USA
//       http://doc.ece.uci.edu/
// and
//       Institute for Software Integrated Systems
//       Vanderbilt University
//       Nashville, TN
//       USA
//       http://www.isis.vanderbilt.edu/
//
// Information about TAO is available at:
//     http://www.cs.wustl.edu/~schmidt/TAO.html

// TAO_IDL - Generated from
// be/be_codegen.cpp:910

#include "DataFlowI.h"
#include "DataFlowS.h"
#include "../PortInterface.hpp"
#include "../ConnectionInterface.hpp"
#include "../Logger.hpp"
#include "CorbaPort.hpp"
#include "ControlTaskProxy.hpp"


using namespace RTT;
using namespace RTT::Corba;


// Implementation skeleton constructor
RTT_Corba_DataFlowInterface_i::RTT_Corba_DataFlowInterface_i (::RTT::DataFlowInterface* dfi)
    : mdf(dfi)
{
}

// Implementation skeleton destructor
RTT_Corba_DataFlowInterface_i::~RTT_Corba_DataFlowInterface_i (void)
{
}

::RTT::Corba::DataFlowInterface::PortNames * RTT_Corba_DataFlowInterface_i::getPorts (

  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    ::RTT::DataFlowInterface::PortNames ports = mdf->getPortNames();

    RTT::Corba::DataFlowInterface::PortNames_var pn = new RTT::Corba::DataFlowInterface::PortNames();
    pn->length( ports.size() );

    for (unsigned int i=0; i != ports.size(); ++i )
        pn[i] = CORBA::string_dup( ports[i].c_str() );

    return pn._retn();
}

::RTT::Corba::AssignableExpression_ptr RTT_Corba_DataFlowInterface_i::createDataChannel (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
    RTT::Logger::In in("createDataChannel");
  // Add your implementation here
    PortInterface* p = mdf->getPort(port_name);

    if ( p == 0) {
        RTT::log() << "No such Port: "<< port_name <<endlog(Error);
        return 0;
    }
    else if (p->getConnectionModel() != PortInterface::Data) {
	RTT::log(Error) << port_name << " is not a data port" << RTT::endlog(Error);
	return 0;
    }

    //use this->isConnected() to thoroughly check the liveness of the connection.
    RTT::ConnectionInterface::shared_ptr ci;
    if ( this->isConnected(port_name) == false) {
        ci = p->createConnection();
        // A newly created connection starts unconnected.
        if (ci)
            ci->connect();
    } else {
        ci = p->connection();
    }

    if ( !ci ) {
        RTT::log() << "Failed to create CORBA Data Connection for Port: "<< port_name <<endlog(Error);
        return 0;
    }
    else if ( ci->getDataSource()->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID) == 0 )
    {
        RTT::log() << "CORBA transport unavailable in data connection for port: "<< port_name << endlog(Error);
        return 0;
    }

    ::RTT::Corba::Expression_var ret = static_cast<Expression_ptr>(ci->getDataSource()->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID)->dataServer( ci->getDataSource(), 0 ));

    ::RTT::Corba::AssignableExpression_var ec = ::RTT::Corba::AssignableExpression::_narrow( ret.in() );

    if ( !CORBA::is_nil(ec) )
        return ec._retn();

    RTT::log() << "Could not create DataChannel for Port (try BufferChannel ?): "<< port_name <<endlog(Error);
    return 0;
}

::RTT::Corba::BufferChannel_ptr RTT_Corba_DataFlowInterface_i::createBufferChannel (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
    RTT::Logger::In in("createBufferChannel");
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0) {
        RTT::log() << "No such Port: "<< port_name <<endlog(Error);
        return 0;
    }
    else if (p->getConnectionModel() != PortInterface::Buffered) {
        RTT::log(Error) << port_name << " is not a buffer port" << RTT::endlog(Error);
        return 0;
    }

    RTT::ConnectionInterface::shared_ptr ci;
    if ( this->isConnected(port_name) == false) {
        ci = p->createConnection();
        // A newly created connection starts unconnected.
        if (ci)
            ci->connect();
    } else {
        ci = p->connection();
    }

    // use the datasource to obtain the protocol.
    if ( !ci ) {
        RTT::log() << "Failed to create CORBA Buffer Connection for Port: "<< port_name <<endlog(Error);
        return 0;
    }
    else if ( ci->getDataSource()->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID) == 0 )
    {
        RTT::log() << "CORBA transport unavailable for connection on port: "<< port_name << endlog(Error);
        return 0;
    }

    // use the getBuffer method to obtain the buffer.
    ::RTT::Corba::BufferChannel_var ret = static_cast<BufferChannel_ptr>(ci->getDataSource()->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID)->bufferServer( ci->getBuffer(), 0 ));

    if ( !CORBA::is_nil(ret) )
        return ret._retn();
    RTT::log() << "Could not create BufferChannel for Port (try DataChannel?): "<< port_name <<endlog(Error);
    return 0;
}

::RTT::Corba::Expression_ptr RTT_Corba_DataFlowInterface_i::createDataObject (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
    RTT::Logger::In in("createDataObject");
  // Add your implementation here
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0) {
        RTT::log() << "No such Port: "<< port_name <<endlog(Error);
        return 0;
    }
    RTT::ConnectionInterface::shared_ptr ci;
    if ( this->isConnected(port_name) == false) {
        RTT::log() << "Can not create DataObject for unconnected Port: "<< port_name <<endlog(Error);
        return 0;
    } else {
        ci = p->connection();
    }

    ::RTT::Corba::Expression_var ret = (::RTT::Corba::Expression_ptr)ci->getDataSource()->server(ORO_CORBA_PROTOCOL_ID, 0);
    if ( CORBA::is_nil( ret ) ) {
        RTT::log() << "Could not create DataObject for connected Port: "<< port_name <<endlog(Error);
        return 0;
    }
    return ret._retn();
}

::RTT::Corba::DataFlowInterface::ConnectionModel RTT_Corba_DataFlowInterface_i::getConnectionModel (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
    PortInterface* p = mdf->getPort(port_name);
    if (p == 0)
        return ::RTT::Corba::DataFlowInterface::ConnectionModel();
    return ::RTT::Corba::DataFlowInterface::ConnectionModel( int(p->getConnectionModel()) );
}


::RTT::Corba::DataFlowInterface::PortType RTT_Corba_DataFlowInterface_i::getPortType (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return ::RTT::Corba::DataFlowInterface::PortType();
    return ::RTT::Corba::DataFlowInterface::PortType( int(p->getPortType()) );
}

char* RTT_Corba_DataFlowInterface_i::getDataType (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
  // Add your implementation here
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return CORBA::string_dup( detail::DataSourceTypeInfo<detail::UnknownType>::getType().c_str() );
    return CORBA::string_dup( p->getTypeInfo()->getTypeName().c_str() );
}

CORBA::Boolean RTT_Corba_DataFlowInterface_i::isConnected (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
  // Add your implementation here
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return 0;
    // this is CORBA: double check if connection is really real.
    try {
        // if the connection points to dead data, this throws TRANSIENT.
        if ( p->connected() && p->connection()->getDataSource()->evaluate() ) {
            return true;
        }
    } catch(...) {
        // evaluate() threw, so disconnect
        // note: should we disconnect the port or the complete connection() ?
        p->connection()->disconnect();
    }
    assert( p->connected() == false);
    return false;
}

void RTT_Corba_DataFlowInterface_i::disconnect (
    const char * port_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return;
    p->disconnect();
}

CORBA::Boolean RTT_Corba_DataFlowInterface_i::connectDataPort (
     const char * port_name,
     ::RTT::Corba::AssignableExpression_ptr data
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ))
{
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return 0;
    else if (p->getConnectionModel() != PortInterface::Data)
        RTT::log(Error) << port_name << " is not a data port" << RTT::endlog(Error);

    // create a proxy to data
    detail::TypeTransporter* tt = p->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID);
    if (tt) {
        DataSourceBase::shared_ptr data_impl( tt->dataProxy( data ) );
        assert(data_impl);
        RTT::ConnectionInterface::shared_ptr ci = p->createConnection ( data_impl );
        assert(ci);
        ci->connect();
        return ci->connected();
    }
    return false;
}

CORBA::Boolean RTT_Corba_DataFlowInterface_i::connectBufferPort (
     const char * port_name,
     ::RTT::Corba::BufferChannel_ptr buffer
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ))
{
    PortInterface* p = mdf->getPort(port_name);
    if ( p == 0)
        return 0;
    else if (p->getConnectionModel() != PortInterface::Buffered)
        RTT::log(Error) << port_name << " is not a buffer port" << RTT::endlog(Error);

    // create a proxy to buffer
    detail::TypeTransporter* tt = p->getTypeInfo()->getProtocol(ORO_CORBA_PROTOCOL_ID);
    if (tt) {
        BufferBase::shared_ptr buf_impl( tt->bufferProxy( buffer ) );

        RTT::ConnectionInterface::shared_ptr ci = p->createConnection ( buf_impl );
        return ci->connect();
    }
    return false;
}

CORBA::Boolean RTT_Corba_DataFlowInterface_i::connectPorts (
     const char * local_name,
     ::RTT::Corba::DataFlowInterface_ptr remote_ports,
     const char * remote_name
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ))
{
    PortInterface* p = mdf->getPort(local_name);
    if ( p == 0)
        return false;

    // Create a helpr proxy object and use the common C++ calls to connect to that proxy.
    ::RTT::Corba::CorbaPort cport( remote_name, remote_ports, ControlTaskProxy::ProxyPOA() ) ;
    PortInterface::ConnectionModel remote_model = cport.getConnectionModel();
    if (remote_model != p->getConnectionModel())
    {
        RTT::log(Error) << "Incompatible connection models between ports "<<local_name<< " and "<<remote_name << RTT::endlog();
        return 0;
    }

    return p->connectTo( &cport );
}

